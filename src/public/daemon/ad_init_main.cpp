#include "../lib/ad_rpc.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include "../gen_code/cpp/public_idl_types.h"
#include "../gen_code/cpp/public_service.h"
#include "../lib/al_utils.h"
static void rerun_config()
{
    AD_RPC_SC::get_instance()->non_block_system("/bin/ad_cli /database/init.txt");
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

public:
    SUBPROCESS_EVENT_SC_NODE(const std::string &_path, const std::vector<std::string> &_argv, const std::string &_name, int _pid)
        : m_path(_path), m_argv(_argv), m_name(_name), m_pid(_pid)
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
            AD_RPC_SC::get_instance()->yield_by_timer(2);
            auto new_pid = create_sub_process(m_path, m_argv);
            close(m_fd);
            if (new_pid > 0)
            {
                m_fd = syscall(SYS_pidfd_open, new_pid, 0);
                m_pid = new_pid;
                set_start_time(al_utils::ad_utils_date_time().m_datetime_ms);
                AD_RPC_SC::get_instance()->registerNode(shared_from_this());
                rerun_config();
            }
        }
    }
};

typedef std::shared_ptr<SUBPROCESS_EVENT_SC_NODE> SUBPROCESS_EVENT_SC_NODE_PTR;
static std::vector<SUBPROCESS_EVENT_SC_NODE_PTR> g_subprocess_list;

static void start_daemon(const std::string &_path, const std::vector<std::string> &_argv, const std::string &_name)
{
    auto exec_args = _argv;
    exec_args.insert(exec_args.begin(), _name);

    auto args_size = exec_args.size();
    auto pid = create_sub_process(_path, exec_args);
    if (pid > 0)
    {
        auto node = std::make_shared<SUBPROCESS_EVENT_SC_NODE>(_path, exec_args, _name, pid);
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
    DaemonService(const std::string &_path, const std::vector<std::string> &_args, const std::string &_name, DaemonService *_depends_on = nullptr)
        : path(_path), args(_args), name(_name), depends_on(_depends_on)
    {
    }
};

static std::vector<DaemonService *> make_init_daemon_services()
{
    std::vector<DaemonService *> services;

    auto log_service = new DaemonService("/bin/log_daemon", {}, "log_daemon");
    services.push_back(log_service);
    auto modbus_service =new DaemonService("/bin/modbus_io_daemon", {}, "modbus_io_daemon", log_service);
    services.push_back(modbus_service);
    auto sm_service = new DaemonService("/bin/sm_daemon", {}, "sm_daemon", modbus_service);
    services.push_back(sm_service);
    auto lidar_service = new DaemonService("/bin/lidar_daemon", {}, "lidar_daemon", sm_service);
    services.push_back(lidar_service);
    auto xlrd_service = new DaemonService("/bin/xlrd_daemon", {}, "xlrd_daemon", sm_service);
    services.push_back(xlrd_service);
    auto live_camera_service = new DaemonService("/bin/live_camera_daemon", {}, "live_camera_daemon", log_service);
    services.push_back(live_camera_service);

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
    start_daemon(_service->path, _service->args, _service->name);
    _service->was_started = true;
}
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
};
int main(int argc, char const *argv[])
{
    auto services = make_init_daemon_services();
    for (auto service : services)
    {
        start_all_daemons(service);
        delete service;
    }
    auto sc = AD_RPC_SC::get_instance();
    sc->add_co(rerun_config);
    sc->enable_rpc_server(AD_RPC_PROCESS_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<public_serviceProcessor>(std::make_shared<public_service_imp>()));
    sc->start_server();

    return 0;
}
