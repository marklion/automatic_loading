#include "lidar_cli.h"
#include "lidar_lib.h"
#include "../../public/lib/CJsonObject.hpp"
#include "../../state_machine/lib/state_machine_lib.h"

static void cap_ply(std::ostream &out, std::vector<std::string> _params)
{
    ad_lidar::call_sm_remote(
        [&](lidar_serviceClient &client)
        {
            ply_file_info ply_info;
            client.cap_current_ply(ply_info, "");
            neb::CJsonObject output;
            output.Add(ply_info.drop_file_path);
            output.Add(ply_info.drop_full_file_path);
            output.Add(ply_info.tail_file_path);
            output.Add(ply_info.tail_full_file_path);
            out << output.ToString() << std::endl;
        });
}
static void turn_on_off(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定lidar开关状态(on/off)");
    if (check_resp.empty())
    {
        bool is_on = (_params[0] == "on");
        std::string kit_name;
        if (_params[0] == "on")
        {
            check_resp = common_cli::check_params(_params, 1, "请指定参数套件");
        }
        if (check_resp.empty())
        {
            kit_name = is_on ? _params[1] : "";
            if (is_on)
            {
                state_machine::call_sm_remote(
                    [&](state_machine_serviceClient &client)
                    {
                        client.apply_config_kit(kit_name);
                    });
            }
            ad_lidar::call_sm_remote(
                [&](lidar_serviceClient &client)
                {
                    client.turn_on_off_lidar(is_on);
                });
        }
        else
        {
            out << check_resp << std::endl;
        }
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void run_against_file(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定文件");
    check_resp += common_cli::check_params(_params, 1, "请指定雷达编号");
    if (check_resp.empty())
    {
        ad_lidar::call_sm_remote(
            [&](lidar_serviceClient &client)
            {
            run_result resp;
            client.run_against_file( resp, _params[0], atoi(_params[1].c_str()));
            out << "distance: " << resp.distance << ", side_z: " << resp.side_z << ", file_name: " << resp.file_name << std::endl; });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> lidar_menu(new cli::Menu("lidar"));
    lidar_menu->Insert(CLI_MENU_ITEM(cap_ply), "捕获当前点云", {});
    lidar_menu->Insert(CLI_MENU_ITEM(turn_on_off), "手动开关雷达", {"on/off", "[kit_name]"});
    lidar_menu->Insert(CLI_MENU_ITEM(run_against_file), "基于文件运行算法", {"<file>", "<lidar_number>"});
    return lidar_menu;
}

lidar_cli::lidar_cli() : common_cli(make_menu(), "lidar")
{
}

std::string lidar_cli::make_bdr()
{
    return std::string();
}

void lidar_cli::clear()
{
}
