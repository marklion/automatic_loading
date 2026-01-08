#include "state_machine_cli.h"
#include "state_machine_lib.h"
#include "../../public/lib/CJsonObject.hpp"

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
            out << "当前状态: " << status.status << std::endl;
            out << "重量: " << status.current_load << std::endl;
            out << "满载偏移: " << status.stuff_full_offset << std::endl;
            out << "车头位置: " << status.vehicle_front_x << std::endl;
            out << "车尾位置: " << status.vehicle_tail_x << std::endl;
            out << "车牌号: " << status.v_info.plate << std::endl;
            out << "货物名称: " << status.v_info.stuff_name << std::endl;
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

static std::unique_ptr<cli::Menu> make_menu()
{
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

    return sm_menu;
}

state_machine_cli::state_machine_cli() : common_cli(make_menu(), "state_machine")
{
}

std::string state_machine_cli::make_bdr()
{
    std::string ret;
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
        });
}
