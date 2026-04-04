#include "../gen_code/cpp/live_camera_idl_types.h"
#include "../gen_code/cpp/live_camera_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../config/lib/config_lib.h"
#include "../../public/lib/al_utils.h"
#include "../../log/lib/log_lib.h"
#include <fstream>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <mutex>

int g_server_pid = 0;
static std::unique_ptr<AD_CO_MUTEX> g_mutex = AD_RPC_SC::get_instance()->create_co_mutex();

static void refresh_live_server()
{
    AD_CO_LOCK_GUARD lock(*g_mutex);
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
    al_log::log_tool m_logger;
    std::map<std::string, video_download_progress> m_video_download_progress;
    AD_CO_MUTEX m_video_download_progress_mutex;
    void put_video_progress(const std::string &_file_name, int _progress)
    {
        AD_CO_LOCK_GUARD lock(m_video_download_progress_mutex);
        video_download_progress tmp;
        tmp.name = _file_name;
        tmp.progress = _progress;
        m_video_download_progress[_file_name] = tmp;
    }

public:
    live_camera_imp() : m_logger(al_log::LOG_LIVE_CAMERA), m_video_download_progress_mutex(AD_RPC_SC::get_instance()) {};
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
        std::string config_file_content = "hlsAlwaysRemux: true\n";
        config_file_content += "hlsSegmentCount:  7\n";
        config_file_content += "hlsSegmentDuration: 1s \n";

        config_file_content += "paths:\n";
        std::vector<live_stream_config> cameras;
        get_all_live_cameras(cameras);
        for (const auto &camera : cameras)
        {
            config_file_content += "  " + camera.name + ":\n    ";
            auto username = al_utils::URLCodec::encode(camera.username, false);
            auto password = al_utils::URLCodec::encode(camera.password, false);
            config_file_content += "source: \"rtsp://" + username + ":" + password + "@" + camera.ip + ":554/Streaming/Channels/" + camera.channel + "\"\n    ";
            config_file_content += "sourceProtocol: tcp\n    sourceOnDemand: yes\n";

            auto replay_channel = camera.channel;
            replay_channel[replay_channel.size() - 1] = '1';
            config_file_content += "  ~" + camera.name + "_replay_start_(\\d{8}T\\d{6}Z)_end_(\\d{8}T\\d{6}Z):\n    ";
            config_file_content += "source: \"rtsp://" + username + ":" + password + "@" + camera.ip + ":554/Streaming/tracks/" + replay_channel + "?starttime=$G1&endtime=$G2\"\n    ";
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

    virtual void generate_video(const std::string &name, const std::string &begin_time, const std::string &end_time)
    {
        std::vector<live_stream_config> cameras;
        get_all_live_cameras(cameras);
        for (const auto &camera : cameras)
        {
            if (camera.name == name)
            {
                auto first_0_pos = camera.channel.find_first_of('0');
                auto real_channel = atoi((camera.channel.substr(0, first_0_pos)).c_str()) - 1;
                std::string file_name = camera.ip + "_" + camera.channel + "_" + al_utils::ad_utils_date_time::make_utc_time(al_utils::ad_utils_date_time().m_datetime) + ".mp4";
                char video_cmd[512] = {0};
                snprintf(
                    video_cmd,
                    sizeof(video_cmd),
                    "/bin/hk_tool '%s' '%s' '%s' '%d' '%s' '%s' '%s'",
                    camera.ip.c_str(),
                    camera.username.c_str(),
                    camera.password.c_str(),
                    real_channel, begin_time.c_str(), end_time.c_str(),
                    file_name.c_str());
                AD_RPC_SC::get_instance()->non_block_system(video_cmd);
                std::ifstream ifs("/database/video/" + file_name);
                if (ifs.good())
                {
                    put_video_progress(file_name, 100);
                }
                else
                {
                    put_video_progress(file_name, -1);
                }
                break;
            }
        }
    }

    virtual void get_video_download_progress(std::vector<video_download_progress> &_return)
    {
        std::vector<std::string> need_delete;
        {
            AD_CO_LOCK_GUARD lock(m_video_download_progress_mutex);
            for (const auto &item : m_video_download_progress)
            {
                auto date_pos = item.second.name.find_last_of('_');
                auto create_date = item.second.name.substr(date_pos + 1, 8);
                auto now_date = al_utils::ad_utils_date_time().make_utc_time(al_utils::ad_utils_date_time().m_datetime).substr(0, 8);
                if (create_date != now_date && item.second.progress >= 100)
                {
                    need_delete.push_back(item.first);
                }
                else
                {

                    _return.push_back(item.second);
                }
            }
            for (auto &itr : need_delete)
            {
                m_video_download_progress.erase(itr);
            }
        }

        for (auto &itr : need_delete)
        {
            std::string file_path = "/database/video/" + itr;
            if (remove(file_path.c_str()) != 0)
            {
                m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Failed to delete file: %s\n", file_path.c_str());
            }
        }
    }
};

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    auto lci = std::make_shared<live_camera_imp>();
    sc->enable_rpc_server(AD_RPC_LIVE_STREAM_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<live_camera_serviceProcessor>(lci));
    al_utils::start_server_notify_started("live_camera");
    if (g_server_pid > 0)
    {
        kill(g_server_pid, SIGKILL);
    }

    return 0;
}
