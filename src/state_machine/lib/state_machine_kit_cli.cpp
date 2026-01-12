#include "state_machine_kit_cli.h"
#include "../../config/lib/config_lib.h"
#include "state_machine_lib.h"

static void add_kit_config_cmd(cli::Menu &_menu, const std::string &_item_key, const std::string &_description)
{
    auto cmd_name = "kit_config_" + _item_key;
    _menu.Insert(
        cmd_name,
        [_item_key](std::ostream &out, std::vector<std::string> _params)
        {
            auto check_resp = common_cli::check_params(_params, 0, "请输入套件名称:");
            check_resp += common_cli::check_params(_params, 1, "请输入配置项值:");
            if (check_resp.empty())
            {
                std::string kit_name = _params[0];
                std::string item_value = _params[1];
                state_machine::call_sm_remote(
                    [kit_name, _item_key, item_value](state_machine_serviceClient &client)
                    {
                        client.add_config_kit(kit_name);
                        client.add_kit_item(kit_name, _item_key, item_value);
                    });
            }
            else
            {
                out << check_resp << std::endl;
            }
        },
        "设置套件的<" + _description + ">配置项",
        {"<kit_name> ", "<item_value>"});
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> sm_kit_menu(new cli::Menu("kit"));
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_OPEN_IO, "开始放料按钮名称");
    add_kit_config_cmd(*sm_kit_menu, CONFIG_ITEM_SM_CONFIG_KIT_CLOSE_IO, "停止放料按钮名称");
    return sm_kit_menu;
}

state_machine_kit_cli::state_machine_kit_cli() : common_cli(make_menu(), "kit")
{
}

std::string state_machine_kit_cli::make_bdr()
{
    return std::string();
}

void state_machine_kit_cli::clear()
{
}
