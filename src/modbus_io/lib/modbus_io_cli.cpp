#include "modbus_io_cli.h"
#include "modbus_io_lib.h"
#include "../../public/lib/al_utils.h"

static void add_device(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入设备名称:");
    check_resp += common_cli::check_params(_params, 1, "请输入设备类型(0-输入,1-输出):");
    check_resp += common_cli::check_params(_params, 2, "请输入设备地址:");
    if (check_resp.empty())
    {
        std::string name = _params[0];
        bool is_output = (atoi(_params[1].c_str()) == 1);
        int address = atoi(_params[2].c_str());
        modbus_io::call_remote_modbus_service(
            [&](modbus_io_serviceClient &client)
            {
                client.add_device(name, address, is_output);
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
        modbus_io::call_remote_modbus_service(
            [&](modbus_io_serviceClient &client)
            {
                client.del_device(name);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void list_devices(std::ostream &out, std::vector<std::string> _params)
{
    std::vector<modbus_device> devices;
    modbus_io::call_remote_modbus_service(
        [&](modbus_io_serviceClient &client)
        {
            client.get_all_devices(devices);
        });
    if (_params.size() > 0 && _params[0] == "json")
    {
        neb::CJsonObject output_json;
        for (const auto &device : devices)
        {
            neb::CJsonObject device_json;
            device_json.Add("device_name", device.device_name);
            device_json.Add("channel_id", device.channel_id);
            device_json.Add("is_output", device.is_output, device.is_output);
            device_json.Add("is_opened", device.is_opened, device.is_opened);
            output_json.Add(device_json);
        }
        out << output_json.ToString() << std::endl;
    }
    else
    {
        tabulate::Table table;
        table.add_row({"名称", "通道ID", "是否输出", "当前状态"});
        for (const auto &device : devices)
        {
            table.add_row({device.device_name, std::to_string(device.channel_id), device.is_output ? "Yes" : "No", device.is_opened ? "吸合" : "断开"});
        }
        table.format().multi_byte_characters(true);
        out << table << std::endl;
    }
}

static void set_modbus_tcp(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入主机地址:");
    check_resp += common_cli::check_params(_params, 1, "请输入端口号:");
    check_resp += common_cli::check_params(_params, 2, "请输入设备ID:");
    if (check_resp.empty())
    {
        std::string host_name = _params[0];
        int port = atoi(_params[1].c_str());
        int device_id = atoi(_params[2].c_str());
        modbus_io::call_remote_modbus_service(
            [&](modbus_io_serviceClient &client)
            {
                modbus_tcp_config tmp_config;
                tmp_config.host_name = host_name;
                tmp_config.port = port;
                tmp_config.device_id = device_id;
                client.set_modbus_tcp(tmp_config);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void device_operate(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入设备名称:");
    check_resp += common_cli::check_params(_params, 1, "请输入操作类型(0-查询,1-设置):");
    if (check_resp.empty())
    {
        std::string name = _params[0];
        bool is_set = (atoi(_params[1].c_str()) == 1);
        modbus_io::call_remote_modbus_service(
            [&](modbus_io_serviceClient &client)
            {
                if (is_set)
                {
                    check_resp += common_cli::check_params(_params, 2, "请输入设置状态(0-断开,1-吸合):");
                    if (!check_resp.empty())
                    {
                        out << check_resp << std::endl;
                        return;
                    }
                    bool is_open = (atoi(_params[2].c_str()) == 1);
                    client.device_io_set(name, is_open);
                }
                else
                {
                    bool status = client.device_io_get(name);
                    out << "设备 " << name << " 当前状态: " << (status ? "吸合" : "断开") << std::endl;
                }
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> sm_menu(new cli::Menu("modbus_io"));
    sm_menu->Insert(CLI_MENU_ITEM(add_device), "添加设备", {"<name>", "<type>", "<address>"});
    sm_menu->Insert(CLI_MENU_ITEM(del_device), "删除设备", {"<name>"});
    sm_menu->Insert(CLI_MENU_ITEM(list_devices), "列出所有设备", {"[json]"});
    sm_menu->Insert(CLI_MENU_ITEM(set_modbus_tcp), "设置Modbus TCP参数", {"<host_name>", "<port>", "<device_id>"});
    sm_menu->Insert(CLI_MENU_ITEM(device_operate), "操作设备IO状态", {"<name>", "<0-查询/1-设置>", "[<0-断开/1-吸合>]"});
    return sm_menu;
}
modbus_io_cli::modbus_io_cli() : common_cli(make_menu(), "modbus_io")
{
}

std::string modbus_io_cli::make_bdr()
{
    std::string ret;

    modbus_io::call_remote_modbus_service(
        [&](modbus_io_serviceClient &client)
        {
            std::vector<modbus_device> devices;
            client.get_all_devices(devices);
            for (const auto &device : devices)
            {
                ret += "add_device \"" + device.device_name + "\" " + std::to_string(device.is_output ? 1 : 0) + " " + std::to_string(device.channel_id) + "\n";
            }
            modbus_tcp_config tmp_config;
            client.get_modbus_tcp(tmp_config);
            if (tmp_config.host_name.length() > 0)
            {
                ret += "set_modbus_tcp \"" + tmp_config.host_name + "\" " + std::to_string(tmp_config.port) + " " + std::to_string(tmp_config.device_id) + "\n";
            }
        });
    return ret;
}

void modbus_io_cli::clear()
{
    modbus_io::call_remote_modbus_service(
        [](modbus_io_serviceClient &client)
        {
            std::vector<modbus_device> devices;
            client.get_all_devices(devices);
            for (const auto &device : devices)
            {
                client.del_device(device.device_name);
            }
            modbus_tcp_config tmp_config;
            tmp_config.host_name = "";
            tmp_config.port = 0;
            tmp_config.device_id = 0;
            client.set_modbus_tcp(tmp_config);
        });
}
