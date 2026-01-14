#include "log_cli.h"
#include "log_lib.h"
#include "../../config/lib/config_lib.h"
static void log_file(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入日志文件路径:");
    if (!check_resp.empty())
    {
        out << check_resp << std::endl;
    }
    else
    {
        al_log::call_log_service(
            [_params](log_serviceClient &client)
            {
                client.set_log_file(_params[0]);
            });
    }
}

static bool g_log_open = false;
static AD_CO_ROUTINE_PTR g_log_co;

static void open_log(std::ostream &out, std::vector<std::string> _params)
{
    g_log_open = true;
    if (!g_log_co)
    {
        g_log_co = AD_RPC_SC::get_instance()->add_co(
            [&]()
            {
                int fd = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in server_addr;
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(AD_BUSINESS_LOG_SERVER_PORT);
                server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
                if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                    out << "无法连接到日志服务" << std::endl;
                }
                else
                {
                    char buffer[2048];
                    while (g_log_open)
                    {
                        AD_RPC_SC::get_instance()->yield_by_fd(fd);
                        int length = read(fd, buffer, sizeof(buffer));
                        if (length > 0)
                        {
                            out.write(buffer, length);
                            out.flush();
                        }
                    }
                }
                close(fd);
            });
    }
}
static void close_log(std::ostream &out, std::vector<std::string> _params)
{
    g_log_open = false;
    g_log_co.reset();
}

static void set_log_level(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入日志级别(0-DEBUG,1-INFO,2-WARN,3-ERROR,4-NONE):");
    check_resp += common_cli::check_params(_params, 1, "请输入日志模块(0-CONFIG,1-MODBUS_IO,2-STATE_MACHINE,3-LIDAR,4-TEST):");
    if (!check_resp.empty())
    {
        out << check_resp << std::endl;
    }
    else
    {
        int level_id = atoi(_params[0].c_str());
        int module_id = atoi(_params[1].c_str());
        al_log::call_log_service(
            [level_id, module_id](log_serviceClient &client)
            {
                log_level_info level_info;
                level_info.level_id = level_id;
                level_info.module_id = module_id;
                client.set_log_level(level_info);
            });
    }
}

static void test_log(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请输入测试日志内容:");
    check_resp += common_cli::check_params(_params, 1, "请输入测试日志级别(0-DEBUG,1-INFO,2-WARN,3-ERROR):");
    check_resp += common_cli::check_params(_params, 2, "请输入测试日志模块(0-CONFIG,1-MODBUS-IO, 2-TEST):");
    if (check_resp.empty())
    {
        auto al = al_log::log_tool((al_log::LOG_MODULE)atoi(_params[2].c_str()));
        al.log_print((al_log::LOG_LEVEL)atoi(_params[1].c_str()), "%s", _params[0].c_str());
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> sm_menu(new cli::Menu("log"));
    sm_menu->Insert(CLI_MENU_ITEM(log_file), "设置日志文件路径", {"<file_path>"});
    sm_menu->Insert(CLI_MENU_ITEM(open_log), "打开日志");
    sm_menu->Insert(CLI_MENU_ITEM(close_log), "关闭日志");
    sm_menu->Insert(CLI_MENU_ITEM(set_log_level), "设置日志级别", {"<level> <module>"});
    sm_menu->Insert(CLI_MENU_ITEM(test_log), "测试日志输出", {"<log_msg>", "<level>", "<module>"});
    return sm_menu;
}
log_cli::log_cli() : common_cli(make_menu(), "log")
{
}

std::string log_cli::make_bdr()
{
    std::string ret;

    al_log::call_log_service(
        [&ret](log_serviceClient &client)
        {
            std::string log_file;
            client.get_log_file(log_file);
            if (log_file != "default.log")
            {
                ret += "log_file \"" + log_file + "\"\n";
            }
        });
    al_log::call_log_service(
        [&ret](log_serviceClient &client)
        {
            std::vector<log_level_info> level_infos;
            client.get_log_level(level_infos);
            for (const auto &level_info : level_infos)
            {
                ret += "set_log_level " + std::to_string(level_info.level_id) + " " + std::to_string(level_info.module_id) + "\n";
            }
        });

    return ret;
}

void log_cli::clear()
{
    al_log::call_log_service(
        [](log_serviceClient &client)
        {
            client.set_log_file("default.log");
        });
    al_log::call_log_service(
        [](log_serviceClient &client)
        {
            client.unset_log_level(-1);
        });
    g_log_open = false;
}
