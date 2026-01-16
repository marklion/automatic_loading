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

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> live_camera_menu(new cli::Menu("live_camera"));
    live_camera_menu->Insert(CLI_MENU_ITEM(add_camera), "添加网络摄像头", {"name", "ip", "username", "password", "channel"});
    live_camera_menu->Insert(CLI_MENU_ITEM(del_camera), "删除网络摄像头", {"name"});
    live_camera_menu->Insert(CLI_MENU_ITEM(show_cameras), "显示所有网络摄像头", {});
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
