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

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> sm_menu(new cli::Menu("process"));
    sm_menu->Insert(CLI_MENU_ITEM(list_process), "列出进程");
    sm_menu->Insert(CLI_MENU_ITEM(list_health_records), "列出健康记录", {"[json]"});
    return sm_menu;
}

public_cli::public_cli() : common_cli(make_menu(), "process")
{
}

std::string public_cli::make_bdr()
{
    return std::string();
}

void public_cli::clear()
{
}
