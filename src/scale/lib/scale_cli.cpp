#include "scale_cli.h"
#include "scale_lib.h"
#include "../../public/lib/CJsonObject.hpp"

static void set_params(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定串口设备名称:");
    check_resp += common_cli::check_params(_params, 1, "请指定波特率:");
    check_resp += common_cli::check_params(_params, 2, "请指定重量系数:");
    if (check_resp.empty())
    {
        std::string dev_name = _params[0];
        std::string baud_rate = _params[1];
        double weight_coeff = atof(_params[2].c_str());
        al_scale::call_scale_service(
            [&](scale_serviceClient &client)
            {
                scale_config_params params;
                params.dev_name = dev_name;
                params.baud_rate = baud_rate;
                params.weight_coeff = weight_coeff;
                client.set_params(params);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void read_weight(std::ostream &out, std::vector<std::string> _params)
{
    al_scale::call_scale_service(
        [&](scale_serviceClient &client)
        {
            double weight = client.read_weight();
            neb::CJsonObject output;
            output.Add("weight", weight);
            out << output.ToString() << std::endl;
        });
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> scale_menu(new cli::Menu("scale"));
    scale_menu->Insert(CLI_MENU_ITEM(set_params), "设置称重设备参数", {"<dev_name> <baud_rate> <weight_coeff>"});
    scale_menu->Insert(CLI_MENU_ITEM(read_weight), "读取当前重量", {});
    return scale_menu;
}
scale_cli::scale_cli() : common_cli(make_menu(), "scale")
{
}

std::string scale_cli::make_bdr()
{
    std::string ret;

    al_scale::call_scale_service(
        [&ret](scale_serviceClient &client)
        {
            scale_config_params params;
            client.get_params(params);
            if (params.dev_name.size())
            {
                ret += "set_params \"" + params.dev_name + "\" " + params.baud_rate + " " + std::to_string(params.weight_coeff) + "\n";
            }
        });
    return ret;
}

void scale_cli::clear()
{
    al_scale::call_scale_service(
        [](scale_serviceClient &client)
        {
            scale_config_params params;
            params.dev_name = "";
            params.baud_rate = "";
            params.weight_coeff = 0;
            client.set_params(params);
        });
}
