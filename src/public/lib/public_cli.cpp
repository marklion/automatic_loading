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

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> sm_menu(new cli::Menu("process"));
    sm_menu->Insert(CLI_MENU_ITEM(list_process), "列出进程");
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
