#include "../gen_code/cpp/live_camera_idl_types.h"
#include "../gen_code/cpp/live_camera_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../config/lib/config_lib.h"
#include "../../public/lib/al_utils.h"
#include <fstream>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <mutex>

int g_server_pid = 0;
std::mutex g_mutex;

static void refresh_live_server()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_server_pid > 0)
    {
        kill(g_server_pid, SIGKILL);
        auto fd = syscall(SYS_pidfd_open, g_server_pid, 0);
        if (fd >= 0)
        {
            AD_RPC_SC::get_instance()->yield_by_fd(fd);
            int status;
            waitpid(g_server_pid, &status, 0);
            close(fd);
        }
    }
    int new_pid = fork();
    if (new_pid == 0)
    {
        execl("/bin/mediamtx", "mediamtx", "/conf/mediamtx.yml", nullptr);
        exit(0);
    }
    else if (new_pid > 0)
    {
        g_server_pid = new_pid;
    }
}

class live_camera_imp : public live_camera_serviceIf
{
public:
    virtual void get_all_live_cameras(std::vector<live_stream_config> &_return)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_LIVE_CAMERA_NAME);
        auto camera_config = ci[CONFIG_ITEM_LIVE_CAMERA_NAME].get_children();
        for (const auto &itr : camera_config)
        {
            live_stream_config config;
            auto &item = *itr;
            config.name = item.get_key();
            config.ip = item(CONFIG_ITEM_LIVE_CAMERA_IP);
            config.username = item(CONFIG_ITEM_LIVE_CAMERA_USERNAME);
            config.password = item(CONFIG_ITEM_LIVE_CAMERA_PASSWORD);
            config.channel = item(CONFIG_ITEM_LIVE_CAMERA_CHANNEL);
            _return.push_back(config);
        }
    }
    void prepare_config_file()
    {
        std::string config_file_content = "paths:\n";
        std::vector<live_stream_config> cameras;
        get_all_live_cameras(cameras);
        for (const auto &camera : cameras)
        {
            config_file_content += "  " + camera.name + ":\n    ";
            auto username = al_utils::URLCodec::encode(camera.username, false);
            auto password = al_utils::URLCodec::encode(camera.password, false);
            config_file_content += "source: \"rtsp://" + username + ":" + password + "@" + camera.ip + ":554/" + camera.channel + "\"\n    ";
            config_file_content += "sourceProtocol: tcp\n    sourceOnDemand: yes\n";
        }
        std::ofstream ofs("/conf/mediamtx.yml", std::ios::trunc);
        ofs << config_file_content;
    }
    virtual bool add_live_camera(const live_stream_config &config)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_LIVE_CAMERA_NAME);
        ci[CONFIG_ITEM_LIVE_CAMERA_NAME].set_child(config.name);
        auto &camera_config = ci[CONFIG_ITEM_LIVE_CAMERA_NAME][config.name];
        camera_config.set_child(CONFIG_ITEM_LIVE_CAMERA_IP, config.ip);
        camera_config.set_child(CONFIG_ITEM_LIVE_CAMERA_USERNAME, config.username);
        camera_config.set_child(CONFIG_ITEM_LIVE_CAMERA_PASSWORD, config.password);
        camera_config.set_child(CONFIG_ITEM_LIVE_CAMERA_CHANNEL, config.channel);
        prepare_config_file();
        refresh_live_server();
        return true;
    }
    virtual bool del_live_camera(const std::string &name)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_LIVE_CAMERA_NAME);
        ci[CONFIG_ITEM_LIVE_CAMERA_NAME].remove_child(name);
        std::vector<live_stream_config> cameras;
        get_all_live_cameras(cameras);
        if (cameras.size() > 0)
        {
            prepare_config_file();
            refresh_live_server();
        }
        return true;
    }
};

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_LIVE_STREAM_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<live_camera_serviceProcessor>(std::make_shared<live_camera_imp>()));
    sc->start_server();
    if (g_server_pid > 0)
    {
        kill(g_server_pid, SIGKILL);
    }
    return 0;
}
