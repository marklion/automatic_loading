#include "../lib/ad_rpc.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include "../gen_code/cpp/public_idl_types.h"
#include "../gen_code/cpp/public_service.h"
#include "../lib/al_utils.h"
#include <fstream>
#include "../lib/modbus_driver.h"

static modbus_driver *gp_modbus_driver = nullptr;
static void watch_dog_active()
{
    if (gp_modbus_driver)
    {
        gp_modbus_driver->write_coil("watch_dog", false);
    }
}
static void watch_dog_reset()
{
    if (gp_modbus_driver)
    {
        gp_modbus_driver->write_coil("watch_dog", true);
    }
}
static void rerun_config(const std::string &_module_name = "")
{
    if (_module_name.empty())
    {
        AD_RPC_SC::get_instance()->non_block_system("/bin/ad_cli /database/init.txt");
    }
    else
    {
        std::ifstream all_config_file("/database/init.txt");
        std::ofstream module_config_file("/tmp/" + _module_name + "_init.txt", std::ios::trunc);
        std::string line;
        bool copy_begin = false;
        bool copy_end = false;
        while (std::getline(all_config_file, line))
        {
            if (line == _module_name)
            {
                copy_begin = true;
            }
            if (line == "ad" && copy_begin)
            {
                copy_end = true;
            }
            if (copy_begin)
            {
                module_config_file << line << std::endl;
            }
            if (copy_end)
            {
                break;
            }
        }
        module_config_file.close();
        AD_RPC_SC::get_instance()->non_block_system("/bin/ad_cli /tmp/" + _module_name + "_init.txt");
    }
}

static int create_sub_process(const std::string &_path, const std::vector<std::string> &_argv)
{
    auto pid = fork();
    if (pid <= 0)
    {
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        char **argv = (char **)malloc((_argv.size() + 1) * sizeof(argv));
        for (size_t i = 0; i < _argv.size(); i++)
        {
            argv[i] = (char *)malloc(_argv[i].length() + 1);
            strcpy(argv[i], _argv[i].c_str());
        }
        argv[_argv.size()] = 0;
        execv(_path.c_str(), argv);
        exit(0);
    }
    return pid;
}
class SUBPROCESS_EVENT_SC_NODE : public AD_EVENT_SC_NODE
{
    std::string m_path;
    std::vector<std::string> m_argv;
    std::string m_name;
    int m_pid;
    int m_fd;
    std::string m_start_time;
    std::string m_module_name;

public:
    SUBPROCESS_EVENT_SC_NODE(const std::string &_path, const std::vector<std::string> &_argv, const std::string &_name, int _pid, const std::string &_module_name)
        : m_path(_path), m_argv(_argv), m_name(_name), m_pid(_pid), m_module_name(_module_name)
    {
        m_fd = syscall(SYS_pidfd_open, _pid, 0);
    }
    ~SUBPROCESS_EVENT_SC_NODE()
    {
        close(m_fd);
    }
    void set_start_time(const std::string &_time)
    {
        m_start_time = _time;
    }
    std::string get_start_time() const
    {
        return m_start_time;
    }
    std::string get_process_name() const
    {
        return m_name;
    }
    int get_process_id() const
    {
        return m_pid;
    }
    virtual std::string node_name() const
    {
        return "subprocess";
    }
    virtual int getFd() const override
    {
        return m_fd;
    }
    int getPid() const
    {
        return m_pid;
    }
    virtual void handleEvent() override
    {
        int status;
        AD_RPC_SC::get_instance()->unregisterNode(shared_from_this());
        waitpid(m_pid, &status, WNOHANG);
        bool need_restart = false;
        if (!WIFEXITED(status))
        {
            need_restart = true;
        }
        else
        {
            if (WEXITSTATUS(status) != 0)
            {
                need_restart = true;
            }
        }
        if (need_restart)
        {
            watch_dog_active();
            AD_RPC_SC::get_instance()->yield_by_timer(2);
            auto new_pid = create_sub_process(m_path, m_argv);
            close(m_fd);
            if (new_pid > 0)
            {
                m_fd = syscall(SYS_pidfd_open, new_pid, 0);
                m_pid = new_pid;
                set_start_time(al_utils::ad_utils_date_time().m_datetime_ms);
                AD_RPC_SC::get_instance()->registerNode(shared_from_this());
            }
        }
    }
};

typedef std::shared_ptr<SUBPROCESS_EVENT_SC_NODE> SUBPROCESS_EVENT_SC_NODE_PTR;
static std::vector<SUBPROCESS_EVENT_SC_NODE_PTR> g_subprocess_list;

static void start_daemon(const std::string &_path, const std::vector<std::string> &_argv, const std::string &_name, const std::string &_module_name)
{
    auto exec_args = _argv;
    exec_args.insert(exec_args.begin(), _name);

    auto args_size = exec_args.size();
    auto pid = create_sub_process(_path, exec_args);
    if (pid > 0)
    {
        auto node = std::make_shared<SUBPROCESS_EVENT_SC_NODE>(_path, exec_args, _name, pid, _module_name);
        node->set_start_time(al_utils::ad_utils_date_time().m_datetime_ms);
        AD_RPC_SC::get_instance()->registerNode(node);
        g_subprocess_list.push_back(node);
    }
}

struct DaemonService
{
    std::string path;
    std::vector<std::string> args;
    std::string name;
    bool was_started = false;
    DaemonService *depends_on = nullptr;
    std::string m_module_name;
    DaemonService(const std::string &_path, const std::vector<std::string> &_args, const std::string &_name, const std::string &_module_name, DaemonService *_depends_on = nullptr)
        : path(_path), args(_args), name(_name), depends_on(_depends_on), m_module_name(_module_name)
    {
    }
};

static std::vector<DaemonService *> make_init_daemon_services()
{
    std::vector<DaemonService *> services;

    auto log_service = new DaemonService("/bin/log_daemon", {}, "log_daemon", "log");
    services.push_back(log_service);
    auto modbus_service = new DaemonService("/bin/modbus_io_daemon", {}, "modbus_io_daemon", "modbus_io", log_service);
    services.push_back(modbus_service);
    auto sm_service = new DaemonService("/bin/sm_daemon", {}, "sm_daemon", "state_machine", modbus_service);
    services.push_back(sm_service);
    auto lidar_service = new DaemonService("/bin/lidar_daemon", {}, "lidar_daemon", "lidar", sm_service);
    services.push_back(lidar_service);
    auto xlrd_service = new DaemonService("/bin/xlrd_daemon", {}, "xlrd_daemon", "xlrd", sm_service);
    services.push_back(xlrd_service);
    auto live_camera_service = new DaemonService("/bin/live_camera_daemon", {}, "live_camera_daemon", "live_camera", log_service);
    services.push_back(live_camera_service);
    auto hn_hht_service = new DaemonService("/bin/hht_daemon", {}, "hht_daemon", "hht", log_service);
    services.push_back(hn_hht_service);
    auto plate_gate_service = new DaemonService("/bin/plate_gate_daemon", {}, "plate_gate_daemon", "plate_gate", sm_service);
    services.push_back(plate_gate_service);
    auto scale_service = new DaemonService("/bin/scale_daemon", {}, "scale_daemon", "scale", sm_service);
    services.push_back(scale_service);
    auto ds_service = new DaemonService("/bin/ds_daemon", {}, "ds_daemon", "drop_system", modbus_service);
    services.push_back(ds_service);

    return services;
}

static void start_all_daemons(DaemonService *_service)
{
    if (_service->was_started)
    {
        return;
    }
    if (_service->depends_on)
    {
        start_all_daemons(_service->depends_on);
    }
    start_daemon(_service->path, _service->args, _service->name, _service->m_module_name);
    _service->was_started = true;
}

static watch_dog_info g_watch_dog_info;

class public_service_imp : public public_serviceIf
{
public:
    virtual void get_all_daemon_meta(std::vector<daemon_meta> &_return)
    {
        for (auto &itr : g_subprocess_list)
        {
            daemon_meta meta;
            meta.daemon_name = itr->get_process_name();
            meta.pid = itr->get_process_id();
            meta.start_time = itr->get_start_time();
            _return.push_back(meta);
        }
    }
    virtual void notify_started(const std::string &module_name)
    {
        rerun_config(module_name);
    }

    virtual void record_health(const std::string &except_info, const std::string &module_name)
    {
        auto date_string = al_utils::ad_utils_date_time().m_datetime_ms;
        std::string record = date_string + "," + module_name + "," + except_info + "\n";

        std::ifstream in_file("/database/health_records.txt");
        std::stringstream file_content;
        if (in_file)
        {
            file_content << in_file.rdbuf();
            in_file.close();
        }

        // 重新打开文件并写入（覆盖模式）
        std::ofstream out_file("/database/health_records.txt", std::ios::trunc);
        if (!out_file)
        {
            return;
        }
        out_file << record << file_content.str();
    }
    virtual void get_health_records(std::vector<health_info> &_return)
    {
        std::ifstream health_record_file("/database/health_records.txt");
        std::string line;
        int max_line_number = 10;
        while (std::getline(health_record_file, line) && max_line_number-- > 0)
        {
            auto first_comma_pos = line.find(',');
            auto second_comma_pos = line.find(',', first_comma_pos + 1);
            if (first_comma_pos != std::string::npos && second_comma_pos != std::string::npos)
            {
                health_info info;
                info.record_time = line.substr(0, first_comma_pos);
                info.module_name = line.substr(first_comma_pos + 1, second_comma_pos - first_comma_pos - 1);
                info.except_info = line.substr(second_comma_pos + 1);
                _return.push_back(info);
            }
        }
        health_record_file.close();
    }

    virtual void set_watch_dog_param(const watch_dog_info &info, const bool is_clear)
    {
        g_watch_dog_info = info;
        if (is_clear)
        {
            g_watch_dog_info.serial_dev_name = "";
            g_watch_dog_info.baud_rate = 0;
            g_watch_dog_info.coil_addr = 0;
            if (gp_modbus_driver)
            {
                delete gp_modbus_driver;
                gp_modbus_driver = nullptr;
            }
        }
        else
        {
            if (gp_modbus_driver)
            {
                delete gp_modbus_driver;
                gp_modbus_driver = nullptr;
            }
            gp_modbus_driver = new modbus_driver(
                g_watch_dog_info.serial_dev_name,
                g_watch_dog_info.baud_rate, 1);
            gp_modbus_driver->add_coil_write_meta("watch_dog", g_watch_dog_info.coil_addr);
        }
    }
    virtual void get_watch_dog_param(watch_dog_info &_return)
    {
        _return = g_watch_dog_info;
    }

    virtual void active_watch_dog()
    {
        watch_dog_active();
    }

    virtual void reset_watch_dog()
    {
        watch_dog_reset();
    }
};

int main(int argc, char const *argv[])
{
    int wait_seconds = 0;
    if (argc > 1)
    {
        wait_seconds = atoi(argv[1]);
        if (wait_seconds < 0)
        {
            wait_seconds = 0;
        }
    }

    auto sc = AD_RPC_SC::get_instance();
    sc->add_co(
        [&]()
        {
            auto services = make_init_daemon_services();
            for (auto service : services)
            {
                start_all_daemons(service);
                delete service;
            }
        });
    sc->enable_rpc_server(AD_RPC_PROCESS_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<public_serviceProcessor>(std::make_shared<public_service_imp>()));
    sc->start_server(
        []()
        {
            al_utils::clear_users();
            rerun_config("process");
            watch_dog_reset();
        });

    return 0;
}
