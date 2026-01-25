#include "state_machine_cli.h"
#include "state_machine_lib.h"
#include "../../public/lib/CJsonObject.hpp"
#include "state_machine_kit_cli.h"
#include "../../public/lib/al_utils.h"

state_machine_kit_cli *g_sm_kit_cli = nullptr;
static void kit_config_item(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入配置套件名称:");
    check_resp += common_cli::check_params(_params, 1, "请输入配置项键:");
    check_resp += common_cli::check_params(_params, 2, "请输入配置项值:");
    if (check_resp.empty())
    {
        std::string kit_name = _params[0];
        std::string item_key = _params[1];
        std::string item_value = _params[2];
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.add_config_kit(kit_name);
                client.add_kit_item(kit_name, item_key, item_value);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void kit_delete_item(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入配置套件名称:");
    check_resp += common_cli::check_params(_params, 1, "请输入配置项键:");
    if (check_resp.empty())
    {
        std::string kit_name = _params[0];
        std::string item_key = _params[1];
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.del_kit_item(kit_name, item_key);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void delete_kit(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入配置套件名称:");
    if (check_resp.empty())
    {
        std::string kit_name = _params[0];
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.del_config_kit(kit_name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void list_kits_json(std::ostream &out, std::vector<std::string> _params)
{
    state_machine::call_sm_remote(
        [&](state_machine_serviceClient &client)
        {
            neb::CJsonObject output_json;
            std::vector<config_kit> kits;
            client.get_all_config_kits(kits);
            for (const auto &kit : kits)
            {
                neb::CJsonObject kit_json;
                kit_json.Add("kit_name", kit.kit_name);
                neb::CJsonObject items_json;
                for (const auto &item : kit.config_items)
                {
                    items_json.Add(item.first, item.second);
                }
                kit_json.Add("config_items", items_json);
                output_json.Add(kit_json);
            }
            out << output_json.ToString() << std::endl;
        });
}

static void mock_load(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入模拟负载值:");
    if (check_resp.empty())
    {
        double load = std::stod(_params[0]);
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.push_cur_load(load);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void mock_offset(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入模拟满载偏移值:");
    if (check_resp.empty())
    {
        double offset = std::stod(_params[0]);
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.push_stuff_full_offset(offset);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void mock_front_x(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入模拟车头位置:");
    if (check_resp.empty())
    {
        double front_x = std::stod(_params[0]);
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.push_vehicle_front_position(front_x);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void mock_tail_x(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入模拟车尾位置:");
    if (check_resp.empty())
    {
        double tail_x = std::stod(_params[0]);
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.push_vehicle_tail_position(tail_x);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void show_status(std::ostream &out, std::vector<std::string> _params)
{
    state_machine::call_sm_remote(
        [&](state_machine_serviceClient &client)
        {
            state_machine_status status;
            client.get_state_machine_status(status);
            if (_params.size() > 0 && _params[0] == "json")
            {
                neb::CJsonObject status_json;
                status_json.Add("status", status.status);
                status_json.Add("current_load", status.current_load);
                status_json.Add("stuff_full_offset", status.stuff_full_offset);
                status_json.Add("vehicle_front_x", status.vehicle_front_x);
                status_json.Add("vehicle_tail_x", status.vehicle_tail_x);
                status_json.Add("is_front_dropped", status.is_front_dropped, status.is_front_dropped);
                status_json.Add("side_z", status.side_z);
                neb::CJsonObject v_info_json;
                v_info_json.Add("plate", status.v_info.plate);
                v_info_json.Add("stuff_name", status.v_info.stuff_name);
                status_json.Add("vehicle_info", v_info_json);
                neb::CJsonObject basic_config_json;
                basic_config_json.Add("max_load", status.basic_config.max_load);
                basic_config_json.Add("max_full_offset", status.basic_config.max_full_offset);
                basic_config_json.Add("front_min_x", status.basic_config.front_min_x);
                basic_config_json.Add("front_max_x", status.basic_config.front_max_x);
                basic_config_json.Add("tail_min_x", status.basic_config.tail_min_x);
                basic_config_json.Add("tail_max_x", status.basic_config.tail_max_x);
                status_json.Add("basic_config", basic_config_json);
                status_json.Add("applied_kit", status.applied_kit);
                out << status_json.ToString() << std::endl;
            }
            else
            {
                out << "当前状态: " << status.status << std::endl;
                out << "重量(最大" << status.basic_config.max_load << "): " << status.current_load << std::endl;
                out << "满载偏移(最大" << status.basic_config.max_full_offset << "): " << status.stuff_full_offset << std::endl;
                out << "车头位置(最小" << status.basic_config.front_min_x << ", 最大" << status.basic_config.front_max_x << "): " << status.vehicle_front_x << std::endl;
                out << "车尾位置(最小" << status.basic_config.tail_min_x << ", 最大" << status.basic_config.tail_max_x << "): " << status.vehicle_tail_x << std::endl;
                out << "车牌号: " << status.v_info.plate << std::endl;
                out << "货物名称: " << status.v_info.stuff_name << std::endl;
                out << "工作溜槽:" << (status.is_front_dropped ? "前溜槽" : "后溜槽") << std::endl;
                out << "应用的配置套件: " << status.applied_kit << std::endl;
            }
        });
}
static void mock_vehicle_info(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入车牌号:");
    check_resp += common_cli::check_params(_params, 1, "请输入货物名称:");
    if (check_resp.empty())
    {
        vehicle_info v_info;
        v_info.plate = _params[0];
        v_info.stuff_name = _params[1];
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.trigger_sm(v_info);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void set_basic_config(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "最大装载重量:");
    check_resp += common_cli::check_params(_params, 1, "最大满载偏移:");
    check_resp += common_cli::check_params(_params, 2, "车头位置最小值:");
    check_resp += common_cli::check_params(_params, 3, "车头位置最大值:");
    check_resp += common_cli::check_params(_params, 4, "车尾位置最小值:");
    check_resp += common_cli::check_params(_params, 5, "车尾位置最大值:");
    if (check_resp.empty())
    {
        sm_basic_config config;
        try
        {
            config.max_load = std::stod(_params[0]);
            config.max_full_offset = std::stod(_params[1]);
            config.front_min_x = std::stod(_params[2]);
            config.front_max_x = std::stod(_params[3]);
            config.tail_min_x = std::stod(_params[4]);
            config.tail_max_x = std::stod(_params[5]);
        }
        catch (...)
        {
            out << "请输入有效的数值参数。" << std::endl;
            return;
        }
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.set_basic_config(config);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void sm_opt(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入操作指令(e-急停, r-重置, m-手动):");
    if (check_resp.empty() && (_params[0] == "e" || _params[0] == "r" || _params[0] == "m"))
    {
        std::string opt = _params[0];
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                if (opt == "e")
                {
                    client.emergency_shutdown();
                }
                else if (opt == "r")
                {
                    client.reset_to_init();
                }
                else if (opt == "m")
                {
                    client.switch_to_manual_mode();
                }
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void default_kit(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入默认配置套件名称:");
    if (check_resp.empty())
    {
        std::string kit_name = _params[0];
        state_machine::call_sm_remote(
            [&](state_machine_serviceClient &client)
            {
                client.set_default_kit(kit_name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    g_sm_kit_cli = new state_machine_kit_cli();
    std::unique_ptr<cli::Menu> sm_menu(new cli::Menu("state_machine"));
    sm_menu->Insert(CLI_MENU_ITEM(kit_config_item), "添加配置项", {"<kit_name>", "<item_key>", "<item_value>"});
    sm_menu->Insert(CLI_MENU_ITEM(kit_delete_item), "删除配置项", {"<kit_name>", "<item_key>"});
    sm_menu->Insert(CLI_MENU_ITEM(delete_kit), "删除配置套件", {"<kit_name>"});
    sm_menu->Insert(CLI_MENU_ITEM(list_kits_json), "列出配置套件", {});
    sm_menu->Insert(CLI_MENU_ITEM(set_basic_config), "设置基本配置", {"<max_load>", "<max_full_offset>", "<front_min_x>", "<front_max_x>", "<tail_min_x>", "<tail_max_x>"});
    sm_menu->Insert(CLI_MENU_ITEM(mock_load), "模拟当前重量", {"<load_value>"});
    sm_menu->Insert(CLI_MENU_ITEM(mock_offset), "模拟满载偏移", {"<offset_value>"});
    sm_menu->Insert(CLI_MENU_ITEM(mock_front_x), "模拟车头位置", {"<front_x>"});
    sm_menu->Insert(CLI_MENU_ITEM(mock_tail_x), "模拟车尾位置", {"<tail_x>"});
    sm_menu->Insert(CLI_MENU_ITEM(show_status), "显示状态机状态", {});
    sm_menu->Insert(CLI_MENU_ITEM(mock_vehicle_info), "模拟车辆信息", {"<plate>", "<stuff_name>"});
    sm_menu->Insert(CLI_MENU_ITEM(sm_opt), "状态机操作", {"<e|r>"});
    sm_menu->Insert(CLI_MENU_ITEM(default_kit), "设置默认配置套件", {"<kit_name>"});
    sm_menu->Insert(std::move(g_sm_kit_cli->menu));

    return sm_menu;
}

state_machine_cli::state_machine_cli() : common_cli(make_menu(), "state_machine")
{
}

std::string state_machine_cli::make_bdr()
{
    std::string ret;
    ret += g_sm_kit_cli->menu_name + "\n";
    ret += al_utils::insert_spaces(g_sm_kit_cli->make_bdr());
    ret += "state_machine\n";
    state_machine::call_sm_remote(
        [&](state_machine_serviceClient &client)
        {
            std::vector<config_kit> kits;
            client.get_all_config_kits(kits);
            for (auto &itr : kits)
            {
                for (auto &item : itr.config_items)
                {
                    ret += "kit_config_item \"" + itr.kit_name + "\" \"" + item.first + "\" \"" + item.second + "\"\n";
                }
            }
            sm_basic_config bc;
            client.get_basic_config(bc);
            if (bc.max_load != 0 && bc.max_full_offset != 0 && bc.front_min_x != 0 && bc.front_max_x != 0 &&
                bc.tail_min_x != 0 && bc.tail_max_x != 0)
            {
                ret +=
                    "set_basic_config " + std::to_string(bc.max_load) + " " +
                    std::to_string(bc.max_full_offset) + " " +
                    std::to_string(bc.front_min_x) + " " +
                    std::to_string(bc.front_max_x) + " " +
                    std::to_string(bc.tail_min_x) + " " +
                    std::to_string(bc.tail_max_x) + "\n";
            }
            std::string default_kit;
            client.get_default_kit(default_kit);
            if (!default_kit.empty())
            {
                ret += "default_kit \"" + default_kit + "\"\n";
            }
        });
    return ret;
}

void state_machine_cli::clear()
{
    state_machine::call_sm_remote(
        [&](state_machine_serviceClient &client)
        {
            std::vector<config_kit> kits;
            client.get_all_config_kits(kits);
            for (auto &itr : kits)
            {
                client.del_config_kit(itr.kit_name);
            }
            sm_basic_config empty_config;
            client.set_basic_config(empty_config);
            client.set_default_kit("");
        });
}
