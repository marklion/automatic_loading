#include "record_cli.h"
#include "record_lib.h"
#include <algorithm>
#include "../../public/lib/al_utils.h"
#include "../sm_gen_code/cpp/state_machine_idl_types.h"
#include "../sm_gen_code/cpp/state_machine_service.h"
#include "../../config/lib/config_lib.h"

static void make_search_params(const std::vector<std::string> &_params, al_record::vehicle_pass_record &_search_param)
{
    if (_params.size() > 0 && _params[0] != "*")
    {
        _search_param.m_plate = _params[0];
    }
    if (_params.size() > 1 && _params[1] != "*")
    {
        _search_param.m_dev_name = _params[1];
    }
    if (_params.size() > 2 && _params[2] != "*")
    {
        _search_param.m_begin_time = _params[2];
    }
    if (_params.size() > 3 && _params[3] != "*")
    {
        _search_param.m_end_time = _params[3];
    }
    if (_params.size() > 4 && _params[4] == "json")
    {
        _search_param.m_is_json = true;
    }
}

static std::string make_replay_url(std::map<std::string, std::string> &_kit_config)
{
    std::string url;

    auto channel = _kit_config.at(CONFIG_ITEM_SM_CONFIG_KIT_VIDEO_NAME);
    url = "/live/" + channel + "_replay";

    return url;
}

static std::map<std::string, std::string> make_url_dev_map()
{
    std::map<std::string, std::string> ret;
    AD_RPC_SC::get_instance()->call_remote<state_machine_serviceClient>(
        AD_RPC_SM_SERVER_PORT,
        [&ret](state_machine_serviceClient &client) {
            std::vector<config_kit> kits;
            client.get_all_config_kits(kits);
            for (auto &kit : kits)
            {
                ret.insert({kit.kit_name, make_replay_url(kit.config_items)});
            }
        });
    return ret;
}

static void list_record(std::ostream &out, std::vector<std::string> _params)
{
    al_record::vehicle_pass_record search_param;
    make_search_params(_params, search_param);
    std::vector<al_record::vehicle_pass_record> record_list;
    al_record::search_vp_list(record_list, search_param.m_plate, search_param.m_begin_time, search_param.m_end_time, search_param.m_dev_name);

    std::sort(
        record_list.begin(),
        record_list.end(),
        [](const al_record::vehicle_pass_record &a, const al_record::vehicle_pass_record &b)
        {
            return al_utils::ad_utils_date_time::is_before(a.m_begin_time, b.m_begin_time);
        });

    if (search_param.m_is_json)
    {
        auto stuff_dev_map = make_url_dev_map();
        neb::CJsonObject json_array("[]");
        for (const auto &record : record_list)
        {
            neb::CJsonObject json_record;
            json_record.Add("plate", record.m_plate);
            json_record.Add("begin_time", record.m_begin_time);
            json_record.Add("end_time", record.m_end_time);
            json_record.Add("dev_name", record.m_dev_name);
            json_record.Add("load", record.m_load);
            json_record.Add(
                "url",
                stuff_dev_map[record.m_dev_name] +
                "_start_" +
                al_utils::ad_utils_date_time::make_utc_time(record.m_begin_time) +
                "_end_" +
                al_utils::ad_utils_date_time::make_utc_time(record.m_end_time) + "/"
            );
            json_array.Add(json_record);
        }
        out << json_array.ToString() << std::endl;
    }
    else
    {
        tabulate::Table table;
        table.add_row({"开始时间", "结束时间", "车牌号", "设备名称", "载重"});
        for (const auto &itr : record_list)
        {
            table.add_row({itr.m_begin_time, itr.m_end_time, itr.m_plate, itr.m_dev_name, al_utils::double2string(itr.m_load)});
        }
        table.format().multi_byte_characters(true);
        out << table << std::endl;
    }
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> record_menu(new cli::Menu("record"));
    record_menu->Insert(CLI_MENU_ITEM(list_record), "查询记录", {"[车牌号]", "[设备名称]", "[开始时间]", "[结束时间]"});
    return record_menu;
}
record_cli::record_cli() : common_cli(make_menu(), "record")
{
}

std::string record_cli::make_bdr()
{
    return std::string();
}

void record_cli::clear()
{
}
