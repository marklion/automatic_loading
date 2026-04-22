#include "public_cli.h"
#include "al_utils.h"
static void list_process(std::ostream &out, std::vector<std::string> _params)
{
    auto metas = al_utils::get_all_daemon_meta();
    for (auto &meta : metas)
    {
        out << "Daemon Name: " << meta->daemon_name << ", PID: " << meta->pid << ", Start Time: " << meta->start_time << std::endl;
    }
}

static void list_health_records(std::ostream &out, std::vector<std::string> _params)
{
    std::vector<health_info> records;
    al_utils::get_health_records(records);
    if (_params.size() > 0 && _params[0] == "json")
    {
        neb::CJsonObject json_array("[]");
        for (const auto &record : records)
        {
            neb::CJsonObject json_record;
            json_record.Add("record_time", record.record_time);
            json_record.Add("module_name", record.module_name);
            json_record.Add("except_info", record.except_info);
            json_array.Add(json_record);
        }
        out << json_array.ToString() << std::endl;
    }
    else
    {
        for (const auto &record : records)
        {
            out << "Time: " << record.record_time << ", Module: " << record.module_name << ", Exception Info: " << record.except_info << std::endl;
        }
    }
}

static void add_user(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定用户名");
    check_resp += common_cli::check_params(_params, 1, "请指定密码");
    if (check_resp.empty())
    {
        al_utils::add_user(_params[0], _params[1]);
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void del_user(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定用户名");
    if (check_resp.empty())
    {
        al_utils::del_user(_params[0]);
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void verify_user(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定用户名");
    check_resp += common_cli::check_params(_params, 1, "请指定密码");
    if (check_resp.empty())
    {
        bool verified = al_utils::verify_user(_params[0], _params[1]);
        neb::CJsonObject json;
        json.Add("verified", verified, verified);
        out << json.ToString() << std::endl;
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> sm_menu(new cli::Menu("process"));
    sm_menu->Insert(CLI_MENU_ITEM(list_process), "列出进程");
    sm_menu->Insert(CLI_MENU_ITEM(list_health_records), "列出健康记录", {"[json]"});
    sm_menu->Insert(CLI_MENU_ITEM(add_user), "添加用户", {"<username>", "<password>"});
    sm_menu->Insert(CLI_MENU_ITEM(del_user), "删除用户", {"<username>"});
    sm_menu->Insert(CLI_MENU_ITEM(verify_user), "验证用户", {"<username>", "<password>"});
    return sm_menu;
}

public_cli::public_cli() : common_cli(make_menu(), "process")
{
}

std::string public_cli::make_bdr()
{
    std::string ret;
    auto users = al_utils::list_users();
    for (const auto &user : users)
    {
        ret += "add_user '" + user.username + "' '" + user.password + "'\n";
    }

    return ret;
}

void public_cli::clear()
{
    auto users = al_utils::list_users();
    for (const auto &user : users)
    {
        al_utils::del_user(user.username);
    }
}
