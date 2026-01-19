#include "../gen_code/cpp/plate_gate_idl_types.h"
#include "../gen_code/cpp/plate_gate_service.h"
#include "plate_gate_cli.h"
#include "../../public/lib/ad_rpc.h"

static void call_pg_remote(std::function<void(plate_gate_serviceClient &)> _func)
{
    AD_RPC_SC::get_instance()->call_remote<plate_gate_serviceClient>(
        AD_RPC_PLATE_GATE_SERVER_PORT, _func);
}
static void set_params(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定设备IP");
    if (check_resp.empty())
    {
        plate_gate_config_params params;
        params.ip = _params[0];
        call_pg_remote(
            [&](plate_gate_serviceClient &client)
            {
                client.set_params(params);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void mock_plate(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定车牌号");
    if (check_resp.empty())
    {
        std::string plate_number = _params[0];
        call_pg_remote(
            [&](plate_gate_serviceClient &client)
            {
                client.plate_capture_notify(plate_number);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void gate_control(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定闸门状态(o-open/c-close)");
    if (check_resp.empty())
    {
        bool is_open = (_params[0] == "o");
        call_pg_remote(
            [&](plate_gate_serviceClient &client)
            {
                client.control_gate(is_open);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> plate_gate_menu(new cli::Menu("plate_gate"));
    plate_gate_menu->Insert(CLI_MENU_ITEM(set_params), "设置设备IP", {"ip"});
    plate_gate_menu->Insert(CLI_MENU_ITEM(mock_plate), "模拟车牌号", {"plate_number"});
    plate_gate_menu->Insert(CLI_MENU_ITEM(gate_control), "闸门控制", {"o-open", "c-close"});
    return plate_gate_menu;
}
plate_gate_cli::plate_gate_cli() : common_cli(make_menu(), "plate_gate")
{
}

std::string plate_gate_cli::make_bdr()
{
    std::string ret;

    call_pg_remote(
        [&](plate_gate_serviceClient &client)
        {
            plate_gate_config_params params;
            client.get_params(params);
            if (params.ip.size() > 0)
            {
                ret += "set_params \"" + params.ip + "\"\n";
            }
        });

    return ret;
}

void plate_gate_cli::clear()
{
    call_pg_remote(
        [&](plate_gate_serviceClient &client)
        {
            plate_gate_config_params params;
            params.ip = "";
            client.set_params(params);
        });
}
