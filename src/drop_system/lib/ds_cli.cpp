#include "ds_cli.h"
#include "../gen_code/cpp/drop_system_idl_types.h"
#include "../gen_code/cpp/drop_system_service.h"
#include "ds_lib.h"
#include "../../public/lib/CJsonObject.hpp"

static void add_device(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入设备名称:");
    check_resp += common_cli::check_params(_params, 1, "请输入设备IP:");
    check_resp += common_cli::check_params(_params, 2, "请输入设备端口:");
    check_resp += common_cli::check_params(_params, 3, "请输入设备Modbus地址:");
    check_resp += common_cli::check_params(_params, 4, "请输入设备最小值:");
    check_resp += common_cli::check_params(_params, 5, "请输入设备最大值:");
    if (check_resp.empty())
    {
        std::string name = _params[0];
        std::string ip = _params[1];
        unsigned short port = atoi(_params[2].c_str());
        int slave_id = atoi(_params[3].c_str());
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                ds_param_info param_info;
                param_info.device_name = name;
                param_info.ip = ip;
                param_info.port = port;
                param_info.slave_id = slave_id;
                param_info.min_value = atof(_params[4].c_str());
                param_info.max_value = atof(_params[5].c_str());
                client.add_param(param_info);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}
static void del_device(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入设备名称:");
    if (check_resp.empty())
    {
        std::string name = _params[0];
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                client.del_param(name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void readout(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入设备名称:");
    if (check_resp.empty())
    {
        std::string name = _params[0];
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                ds_readout readout_result;
                client.readout(readout_result, name);
                out << "设备 " << name << " 当前读数: " << readout_result.value << ", 读数占比: " << readout_result.rate * 100 << "%" << std::endl;
            });
    }
}

static void add_match(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请拉绳传感器设备名称:");
    check_resp += common_cli::check_params(_params, 1, "请输入放料开门按钮名称:");
    check_resp += common_cli::check_params(_params, 2, "请输入放料关门按钮名称:");
    if (check_resp.empty())
    {
        std::string input_name = _params[0];
        std::string output_on_name = _params[1];
        std::string output_off_name = _params[2];
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                ds_input_output output_match;
                output_match.output_on_device_name = output_on_name;
                output_match.output_off_device_name = output_off_name;
                client.add_output_match(input_name, output_match);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void del_match(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请拉绳传感器设备名称:");
    if (check_resp.empty())
    {
        std::string input_name = _params[0];
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                client.del_output_match(input_name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void set_expect(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请拉绳传感器设备名称:");
    check_resp += common_cli::check_params(_params, 1, "请输入期望的读数占比(0-100):");
    if (check_resp.empty())
    {
        std::string input_name = _params[0];
        double expect_rate = atof(_params[1].c_str()) / 100;
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                client.set_output(expect_rate, input_name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void turn_on_off(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入on或off:");
    if (check_resp.empty())
    {
        std::string on_off = _params[0];
        bool turn_on = (on_off == "on");
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                client.turn_on_off(turn_on);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void show_status(std::ostream &out, std::vector<std::string> _params)
{
    drop_system::call_remote_ds(
        [&](drop_system_serviceClient &client)
        {
            neb::CJsonObject status_json("[]");
            std::vector<ds_param_info> param_infos;
            client.get_all_params(param_infos);
            for (auto &itr:param_infos)
            {
                ds_readout tmp;
                client.readout(tmp, itr.device_name);
                neb::CJsonObject one_dev;
                one_dev.Add("device_name", itr.device_name);
                one_dev.Add("value", tmp.value);
                one_dev.Add("rate", tmp.rate);
                status_json.Add(one_dev);
            }
            out << status_json.ToString() << std::endl;
        });
}

static void one_time_drop(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请拉绳传感器设备名称:");
    if (check_resp.empty())
    {
        std::string input_name = _params[0];
        drop_system::call_remote_ds(
            [&](drop_system_serviceClient &client)
            {
                client.open_one_time(input_name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> ds_menu(new cli::Menu("drop_system"));
    ds_menu->Insert(CLI_MENU_ITEM(add_device), "添加设备", {"<device_name>", "<ip>", "<port>", "<slave_id>"});
    ds_menu->Insert(CLI_MENU_ITEM(del_device), "删除设备", {"<device_name>"});
    ds_menu->Insert(CLI_MENU_ITEM(readout), "读取设备数据", {"<device_name>"});
    ds_menu->Insert(CLI_MENU_ITEM(add_match), "添加匹配", {"<input_device_name>", "<output_on_device_name>", "<output_off_device_name>"});
    ds_menu->Insert(CLI_MENU_ITEM(del_match), "删除匹配", {"<input_device_name>"});
    ds_menu->Insert(CLI_MENU_ITEM(set_expect), "设置期望值", {"<input_device_name>", "<expect_rate>"});
    ds_menu->Insert(CLI_MENU_ITEM(turn_on_off), "开关控制", {"<on|off>"});
    ds_menu->Insert(CLI_MENU_ITEM(show_status), "显示状态", {});
    ds_menu->Insert(CLI_MENU_ITEM(one_time_drop), "一次性放料", {"<input_device_name>"});
    return ds_menu;
}
ds_cli::ds_cli() : common_cli(make_menu(), "drop_system")
{
}

std::string ds_cli::make_bdr()
{
    std::string ret;

    drop_system::call_remote_ds(
        [&](drop_system_serviceClient &client)
        {
            std::vector<ds_param_info> param_infos;
            client.get_all_params(param_infos);
            for (const auto &param_info : param_infos)
            {
                ret += "add_device \"" +
                       param_info.device_name + "\" \"" +
                       param_info.ip + "\" " +
                       std::to_string(param_info.port) + " " +
                       std::to_string(param_info.slave_id) + " " +
                       std::to_string(param_info.min_value) + " " +
                       std::to_string(param_info.max_value) + "\n";
            }
            std::vector<ds_input_output> output_matches;
            client.get_all_output_match(output_matches);
            for (const auto &output_match : output_matches)
            {
                ret += "add_match \"" + output_match.input_device_name + "\" \"" + output_match.output_on_device_name + "\" \"" + output_match.output_off_device_name + "\"\n";
            }
            std::string on_off = client.is_turned_on() ? "on" : "off";
            ret += "turn_on_off \"" + on_off + "\"\n";
        });

    return ret;
}

void ds_cli::clear()
{
    drop_system::call_remote_ds(
        [&](drop_system_serviceClient &client)
        {
            std::vector<ds_param_info> param_infos;
            client.get_all_params(param_infos);
            for (const auto &param_info : param_infos)
            {
                client.del_param(param_info.device_name);
            }
            std::vector<ds_input_output> output_matches;
            client.get_all_output_match(output_matches);
            for (const auto &output_match : output_matches)
            {
                client.del_output_match(output_match.input_device_name);
            }
            client.turn_on_off(false);
        });
}
