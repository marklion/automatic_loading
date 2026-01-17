#include "hn_hht_cli.h"
#include "hn_hht_lib.h"

static void set_params(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定AppKey");
    check_resp += common_cli::check_params(_params, 1, "请指定AppSecret");
    if (check_resp.empty())
    {
        std::string app_key = _params[0];
        std::string app_secret = _params[1];
        hn_hht::call_hht_service_remote(
            [&](hn_hht_serviceClient &client)
            {
                hht_config_params params;
                params.app_key = app_key;
                params.app_secret = app_secret;
                client.set_params(params);
            });
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static void test_get_order(std::ostream &out, std::vector<std::string> _params)
{
    auto check_resp = common_cli::check_params(_params, 0, "请指定查询内容");
    if (check_resp.empty())
    {
        std::string query_content = _params[0];
        std::string order = hn_hht::req_get_order(query_content);
        out << order << std::endl;
    }
    else
    {
        out << check_resp << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> hht_menu(new cli::Menu("hht"));
    hht_menu->Insert(CLI_MENU_ITEM(set_params), "设置HHT参数", {"AppKey", "AppSecret"});
    hht_menu->Insert(CLI_MENU_ITEM(test_get_order), "测试获取订单", {"查询内容"});
    return hht_menu;
}

hn_hht_cli::hn_hht_cli() : common_cli(make_menu(), "hht")
{
}

std::string hn_hht_cli::make_bdr()
{
    std::string ret;

    hn_hht::call_hht_service_remote(
        [&](hn_hht_serviceClient &client)
        {
            hht_config_params params;
            client.get_params(params);
            if (params.app_key.size() > 0 || params.app_secret.size() > 0)
            {
                ret += "set_params \"" + params.app_key + "\" \"" + params.app_secret + "\"\n";
            }
        });

    return ret;
}

void hn_hht_cli::clear()
{
    hn_hht::call_hht_service_remote(
        [&](hn_hht_serviceClient &client)
        {
            hht_config_params params;
            params.app_key = "";
            params.app_secret = "";
            client.set_params(params);
        });
}
