#include "record_cli.h"
#include "record_lib.h"
#include <algorithm>
#include "../../public/lib/al_utils.h"
#include "../../config/lib/config_lib.h"
#include "../../live_camera/lib/live_camera_lib.h"

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

static void list_record(std::ostream &out, std::vector<std::string> _params)
{
    al_record::vehicle_pass_record search_param;
    make_search_params(_params, search_param);
    std::vector<al_record::vehicle_pass_record> record_list;
    al_record::search_vp_list(record_list, search_param.m_plate, search_param.m_begin_time, search_param.m_end_time, search_param.m_dev_name);

    if (search_param.m_is_json)
    {
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
                record.make_live_url());
            json_record.Add("video_file_name", record.m_video_file_name);
            json_record.Add("video_download_progress", record.m_video_download_progress);
            json_record.Add("justified", record.m_justified);
            json_array.Add(json_record);
        }
        out << json_array.ToString() << std::endl;
    }
    else
    {
        tabulate::Table table;
        table.add_row({"开始时间", "结束时间", "车牌号", "设备名称", "载重", "视频文件", "下载进度", "是否干预"});
        for (const auto &itr : record_list)
        {
            table.add_row({itr.m_begin_time,
                           itr.m_end_time,
                           itr.m_plate,
                           itr.m_dev_name,
                           al_utils::double2string(itr.m_load),
                           itr.m_video_file_name,
                           std::to_string(itr.m_video_download_progress),
                           itr.m_justified ? "是" : "否"});
        }
        table.format().multi_byte_characters(true);
        out << table << std::endl;
    }
}

static void make_video(std::ostream &out, std::vector<std::string> _params)
{
    std::vector<std::string> string_search_params = _params;
    auto rec_index = atoi(string_search_params.front().c_str());
    if (rec_index < 0)
    {
        out << "无记录" << std::endl;
        return;
    }
    string_search_params.erase(string_search_params.begin());
    al_record::vehicle_pass_record search_param;
    make_search_params(string_search_params, search_param);
    std::vector<al_record::vehicle_pass_record> record_list;
    al_record::search_vp_list(record_list, search_param.m_plate, search_param.m_begin_time, search_param.m_end_time, search_param.m_dev_name);

    if (rec_index >= record_list.size())
    {
        out << "无记录" << std::endl;
        return;
    }
    auto &record = record_list[rec_index];
    if (record.m_video_file_name.length() > 0)
    {
        out << "视频已存在，文件名：" << record.m_video_file_name << std::endl;
        return;
    }
    record.generate_video();
    al_record::refresh_vp(record);
}

static std::unique_ptr<cli::Menu> make_menu()
{
    std::unique_ptr<cli::Menu> record_menu(new cli::Menu("record"));
    record_menu->Insert(CLI_MENU_ITEM(list_record), "查询记录", {"[车牌号]", "[设备名称]", "[开始时间]", "[结束时间]"});
    record_menu->Insert(CLI_MENU_ITEM(make_video), "生成视频", {"记录索引", "[车牌号]", "[设备名称]", "[开始时间]", "[结束时间]"});
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
