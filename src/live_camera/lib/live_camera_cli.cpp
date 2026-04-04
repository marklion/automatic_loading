#include "live_camera_cli.h"
#include "live_camera_lib.h"
#include "../../public/lib/CJsonObject.hpp"

static void add_camera(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定摄像头名称");
    check_resp += common_cli::check_params(_params, 1, "请指定摄像头IP地址");
    check_resp += common_cli::check_params(_params, 2, "请指定摄像头用户名");
    check_resp += common_cli::check_params(_params, 3, "请指定摄像头密码");
    check_resp += common_cli::check_params(_params, 4, "请指定摄像头通道号");
    if (check_resp.empty())
    {
        live_stream_config config;
        config.name = _params[0];
        config.ip = _params[1];
        config.username = _params[2];
        config.password = _params[3];
        config.channel = _params[4];
        live_camera::call_live_camera_remote(
            [&](live_camera_serviceClient &client)
            {
                client.add_live_camera(config);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void del_camera(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定摄像头名称");
    if (check_resp.empty())
    {
        std::string name = _params[0];
        live_camera::call_live_camera_remote(
            [&](live_camera_serviceClient &client)
            {
                client.del_live_camera(name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void show_cameras(std::ostream &out, std::vector<std::string> _params)
{
    neb::CJsonObject camera_json("[]");
    live_camera::call_live_camera_remote(
        [&](live_camera_serviceClient &client)
        {
            std::vector<live_stream_config> cameras;
            client.get_all_live_cameras(cameras);
            for (const auto &camera : cameras)
            {
                neb::CJsonObject single_camera;
                single_camera.Add("name", camera.name);
                single_camera.Add("ip", camera.ip);
                single_camera.Add("username", camera.username);
                single_camera.Add("password", camera.password);
                single_camera.Add("channel", camera.channel);
                camera_json.Add(single_camera);
            }
        });
    out << camera_json.ToString() << std::endl;
}

static void generate_video(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定摄像头名称");
    check_resp += common_cli::check_params(_params, 1, "请指定录像开始时间，格式为2026-01-23 16:33:03");
    check_resp += common_cli::check_params(_params, 2, "请指定录像结束时间，格式为2026-01-23 16:33:03");
    if (check_resp.empty())
    {
        std::string name = _params[0];
        std::string begin_time = _params[1];
        std::string end_time = _params[2];
        live_camera::call_live_camera_remote(
            [&](live_camera_serviceClient &client)
            {
                client.generate_video(name, begin_time, end_time);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void list_video(std::ostream &out, std::vector<std::string> _params)
{
    std::vector<video_download_progress> progress_list;
    live_camera::call_live_camera_remote(
        [&](live_camera_serviceClient &client)
        {
            client.get_video_download_progress(progress_list);
        });
    if (_params.size() > 0 && _params[0] == "json")
    {
        neb::CJsonObject progress_json("[]");
        for (const auto &progress : progress_list)
        {
            neb::CJsonObject single_progress;
            single_progress.Add("name", progress.name);
            single_progress.Add("progress", progress.progress);
            progress_json.Add(single_progress);
        }
        out << progress_json.ToString() << std::endl;
    }
    else
    {
        tabulate::Table table;
        table.add_row({"文件", "进度"});
        for (const auto &itr : progress_list)
        {
            table.add_row({itr.name, std::to_string(itr.progress) + "%"});
        }
        table.format().multi_byte_characters(true);
        out << table << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> live_camera_menu(new cli::Menu("live_camera"));
    live_camera_menu->Insert(CLI_MENU_ITEM(add_camera), "添加网络摄像头", {"name", "ip", "username", "password", "channel"});
    live_camera_menu->Insert(CLI_MENU_ITEM(del_camera), "删除网络摄像头", {"name"});
    live_camera_menu->Insert(CLI_MENU_ITEM(show_cameras), "显示所有网络摄像头", {});
    live_camera_menu->Insert(CLI_MENU_ITEM(generate_video), "生成录像文件", {"name", "begin_time", "end_time"});
    live_camera_menu->Insert(CLI_MENU_ITEM(list_video), "列出录像生成进度", {"[json]"});
    return live_camera_menu;
}

live_camera_cli::live_camera_cli() : common_cli(make_menu(), "live_camera")
{
}

std::string live_camera_cli::make_bdr()
{
    std::string ret;

    live_camera::call_live_camera_remote(
        [&](live_camera_serviceClient &client)
        {
            std::vector<live_stream_config> cameras;
            client.get_all_live_cameras(cameras);
            for (const auto &camera : cameras)
            {
                ret += "add_camera \"" + camera.name + "\" \"" + camera.ip + "\" \"" + camera.username + "\" \"" + camera.password + "\" \"" + camera.channel + "\"\n";
            }
        });

    return ret;
}

void live_camera_cli::clear()
{
    live_camera::call_live_camera_remote(
        [&](live_camera_serviceClient &client)
        {
            std::vector<live_stream_config> cameras;
            client.get_all_live_cameras(cameras);
            for (const auto &camera : cameras)
            {
                client.del_live_camera(camera.name);
            }
        });
}
