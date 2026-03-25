#include "record_cli.h"
#include "record_lib.h"

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
}

static void list_record(std::ostream &out, std::vector<std::string> _params)
{
    al_record::vehicle_pass_record search_param;
    make_search_params(_params, search_param);
    std::vector<al_record::vehicle_pass_record> record_list;
    al_record::search_vp_list(record_list, search_param.m_plate, search_param.m_begin_time, search_param.m_end_time, search_param.m_dev_name);

    tabulate::Table table;
    table.add_row({"开始时间", "结束时间", "车牌号", "设备名称"});
    for (const auto &itr : record_list)
    {
        table.add_row({itr.m_begin_time, itr.m_end_time, itr.m_plate, itr.m_dev_name});
    }
    table.format().multi_byte_characters(true);
    out << table << std::endl;
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
