#include "xlrd_cli.h"
#include "xlrd_lib.h"
#include "../../public/lib/CJsonObject.hpp"

static void set_params(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定物位计编号(0-前，1-后)");
    check_resp += common_cli::check_params(_params, 1, "请指定IP");
    check_resp += common_cli::check_params(_params, 2, "请指定端口号");
    check_resp += common_cli::check_params(_params, 3, "请指定设备地址");
    check_resp += common_cli::check_params(_params, 4, "请指定偏移补偿");
    check_resp += common_cli::check_params(_params, 5, "请指定地面高度坐标");
    if (check_resp.empty())
    {
        bool is_front = (_params[0] == "0");
        std::string ip = _params[1];
        int port = atoi(_params[2].c_str());
        int dev_addr = atoi(_params[3].c_str());
        float offset = atof(_params[4].c_str());
        float ground_height = atof(_params[5].c_str());
        ad_xlrd::call_xlrd_service_remote(
            [&](xlrd_serviceClient &client)
            {
                xlrd_config_params params;
                params.ip = ip;
                params.port = port;
                params.slave_id = dev_addr;
                params.distance_offset = offset;
                params.bottom_z = ground_height;
                client.set_config_params(is_front, params);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void read_offset(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定物位计编号(0-前，1-后)");
    if (check_resp.empty())
    {
        bool is_front = (_params[0] == "0");
        ad_xlrd::call_xlrd_service_remote(
            [&](xlrd_serviceClient &client)
            {
                double distance = client.read_distance(is_front);
                neb::CJsonObject output;
                output.Add("offset", distance);
                out << output.ToString() << std::endl;
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> xlrd_menu(new cli::Menu("xlrd"));
    xlrd_menu->Insert(CLI_MENU_ITEM(set_params), "设置物位计参数", {"0/1", "ip", "port", "dev_addr", "offset", "ground_height"});
    xlrd_menu->Insert(CLI_MENU_ITEM(read_offset), "读取物位计距离偏移", {"0/1"});
    return xlrd_menu;
}

xlrd_cli::xlrd_cli() : common_cli(make_menu(), "xlrd")
{
}

std::string xlrd_cli::make_bdr()
{
    std::string ret;

    ad_xlrd::call_xlrd_service_remote(
        [&](xlrd_serviceClient &client)
        {
            xlrd_config_params front_params;
            xlrd_config_params back_params;
            client.get_config_params(front_params, true);
            client.get_config_params(back_params, false);
            if (front_params.ip.length() > 0)
            {
                ret += "set params 0 \"" + front_params.ip + "\" " + std::to_string(front_params.port) + " " +
                       std::to_string(front_params.slave_id) + " " + std::to_string(front_params.distance_offset) + " " +
                       std::to_string(front_params.bottom_z) + "\n";
            }
            if (back_params.ip.length() > 0)
            {
                ret += "set params 1 \"" + back_params.ip + "\" " + std::to_string(back_params.port) + " " +
                       std::to_string(back_params.slave_id) + " " + std::to_string(back_params.distance_offset) + " " +
                       std::to_string(back_params.bottom_z) + "\n";
            }
        });

    return ret;
}

void xlrd_cli::clear()
{
    ad_xlrd::call_xlrd_service_remote(
        [&](xlrd_serviceClient &client)
        {
            xlrd_config_params empty_params;
            client.set_config_params(true, empty_params);
            client.set_config_params(false, empty_params);
        });
}
