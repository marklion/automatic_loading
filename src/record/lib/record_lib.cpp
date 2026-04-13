#include "record_lib.h"
#include <memory>
#include <fstream>
#include <dirent.h>
#include "../../public/lib/al_utils.h"
#include "../sm_gen_code/cpp/state_machine_idl_types.h"
#include "../sm_gen_code/cpp/state_machine_service.h"
#include "../../config/lib/config_lib.h"
#include "../../public/lib/ad_rpc.h"
#include <algorithm>

#define VP_FILE_NAME_PREFIX "vehicle_pass_record_"
#define RECORD_FILE_PATH_PREFIX "/database/" VP_FILE_NAME_PREFIX

static std::string make_one_line_record(const al_record::vehicle_pass_record &record)
{
    return record.m_plate + "," +
           record.m_begin_time + "," +
           record.m_end_time + "," +
           record.m_dev_name + "," +
           al_utils::double2string(record.m_load) + "," +
           record.m_video_file_name + "," +
           (record.m_justified ? "1" : "0");
}

static std::unique_ptr<al_record::vehicle_pass_record> parse_one_line_record(const std::string &line)
{
    size_t pos1 = line.find(",");
    if (pos1 == std::string::npos)
    {
        return nullptr;
    }
    size_t pos2 = line.find(",", pos1 + 1);
    if (pos2 == std::string::npos)
    {
        return nullptr;
    }
    size_t pos3 = line.find(",", pos2 + 1);
    if (pos3 == std::string::npos)
    {
        return nullptr;
    }
    size_t pos4 = line.find(",", pos3 + 1);
    std::string plate = line.substr(0, pos1);
    std::string begin_time = line.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string end_time = line.substr(pos2 + 1, pos3 - pos2 - 1);
    std::string dev_name = line.substr(pos3 + 1);
    double load = 0;
    std::string file_name;
    bool justified = false;
    if (pos4 != std::string::npos)
    {
        dev_name = line.substr(pos3 + 1, pos4 - pos3 - 1);
        load = atof(line.substr(pos4 + 1).c_str());
        size_t pos5 = line.find(",", pos4 + 1);
        if (pos5 != std::string::npos)
        {
            load = atof(line.substr(pos4 + 1, pos5 - pos4 - 1).c_str());
            file_name = line.substr(pos5 + 1);
            size_t pos6 = line.find(",", pos5 + 1);
            if (pos6 != std::string::npos)
            {
                file_name = line.substr(pos5 + 1, pos6 - pos5 - 1);
                std::string justified_str = line.substr(pos6 + 1);
                justified = justified_str == "1";
            }
        }
    }

    auto ret = std::make_unique<al_record::vehicle_pass_record>(plate, begin_time, end_time, dev_name, load, justified);
    ret->m_video_file_name = file_name;
    return ret;
}

void al_record::record_vehicle_pass(const vehicle_pass_record &record)
{
    auto one_line = make_one_line_record(record);
    auto record_date = record.m_begin_time.substr(0, 10);
    auto file_name = RECORD_FILE_PATH_PREFIX + record_date + ".csv";
    std::ofstream ofs(file_name, std::ios::app);
    ofs << one_line << std::endl;
}

static void search_file_list(std::vector<std::string> &file_list, const std::string &_begin_time, const std::string &_end_time)
{
    auto begin_date = _begin_time.substr(0, 10);
    auto end_date = _end_time.substr(0, 10);
    end_date = al_utils::ad_utils_date_time::date_plus_day(end_date, 1);
    if (begin_date.length() > 0 && end_date.length() > 0)
    {
        for (auto itr_date = begin_date; al_utils::ad_utils_date_time::is_before(itr_date, end_date); itr_date = al_utils::ad_utils_date_time::date_plus_day(itr_date, 1))
        {
            auto file_name = RECORD_FILE_PATH_PREFIX + itr_date + ".csv";
            file_list.push_back(file_name);
        }
    }
    else
    {
        // 如果日期不合法，则查找所有文件PREFIX开头的文件
        // 这种情况一般是查询条件没有时间限制，或者时间格式不对导致日期解析失败
        DIR *dir = opendir("/database");
        if (dir != nullptr)
        {
            struct dirent *ent;
            while ((ent = readdir(dir)) != nullptr)
            {
                std::string filename = ent->d_name;
                if (ent->d_type == DT_REG && filename.find(VP_FILE_NAME_PREFIX) == 0)
                {
                    file_list.push_back(std::string("/database/") + filename);
                }
            }
            closedir(dir);
        }
    }
}

void al_record::search_vp_list(std::vector<vehicle_pass_record> &_return, const std::string &_plate, const std::string &_begin_time, const std::string &_end_time, const std::string &_dev_name)
{
    std::vector<video_download_progress> progress_list;
    live_camera::call_live_camera_remote(
        [&](live_camera_serviceClient &client)
        {
            client.get_video_download_progress(progress_list);
        });
    std::vector<std::string> file_list;
    search_file_list(file_list, _begin_time, _end_time);
    std::vector<vehicle_pass_record> need_refresh_record;
    for (const auto &file_name : file_list)
    {
        std::ifstream ifs(file_name);
        std::string line;
        if (ifs.is_open())
        {
            while (std::getline(ifs, line))
            {
                auto record_ptr = parse_one_line_record(line);
                if (record_ptr)
                {
                    auto &record = *record_ptr;
                    if ((record.m_plate == _plate || _plate.empty()) &&
                        (record.m_dev_name == _dev_name || _dev_name.empty()))
                    {
                        auto tmp = record;
                        tmp.m_video_download_progress = get_progress_by_file_name(record.m_video_file_name, progress_list);
                        if (tmp.m_video_download_progress < 0)
                        {
                            tmp.m_video_file_name = "";
                        }
                        need_refresh_record.push_back(tmp);
                        _return.push_back(tmp);
                    }
                }
            }
        }
    }
    for (auto &record : need_refresh_record)
    {
        refresh_vp(record);
    }
    std::sort(
        _return.begin(),
        _return.end(),
        [](const al_record::vehicle_pass_record &a, const al_record::vehicle_pass_record &b)
        {
            return al_utils::ad_utils_date_time::is_before(a.m_begin_time, b.m_begin_time);
        });
}
void al_record::refresh_vp(const vehicle_pass_record &record)
{
    std::vector<std::string> file_list;
    search_file_list(file_list, record.m_begin_time, record.m_begin_time);
    if (file_list.size() == 1)
    {
        std::string file_name = file_list[0];
        std::vector<vehicle_pass_record> record_list;
        std::ifstream ifs(file_name);
        std::string line;
        if (ifs.is_open())
        {
            while (std::getline(ifs, line))
            {
                auto record_ptr = parse_one_line_record(line);
                if (record_ptr)
                {
                    record_list.push_back(*record_ptr);
                }
            }
        }
        ifs.close();
        std::ofstream ofs(file_name);
        for (const auto &item : record_list)
        {
            if (item.m_plate == record.m_plate &&
                item.m_begin_time == record.m_begin_time &&
                item.m_end_time == record.m_end_time &&
                item.m_dev_name == record.m_dev_name)
            {
                ofs << make_one_line_record(record) << std::endl;
            }
            else
            {
                ofs << make_one_line_record(item) << std::endl;
            }
        }
    }
}
int al_record::get_progress_by_file_name(const std::string &file_name, const std::vector<video_download_progress> &_vdp)
{
    auto found_ret = std::find_if(
        _vdp.begin(), _vdp.end(),
        [&file_name](const video_download_progress &progress)
        { return progress.name == file_name; });
    if (found_ret != _vdp.end())
    {
        return found_ret->progress;
    }
    return -1;
}
static std::string make_replay_url_or_cam_name(std::map<std::string, std::string> &_kit_config, bool _is_cam_name = false)
{
    std::string url;

    auto channel = _kit_config.at(CONFIG_ITEM_SM_CONFIG_KIT_VIDEO_NAME);
    url = "/live/" + channel + "_replay";
    if (_is_cam_name)
    {
        url = channel;
    }

    return url;
}
static std::map<std::string, std::string> make_url_dev_map(bool _is_video = false)
{
    std::map<std::string, std::string> ret;
    AD_RPC_SC::get_instance()->call_remote<state_machine_serviceClient>(
        AD_RPC_SM_SERVER_PORT,
        [&ret, _is_video](state_machine_serviceClient &client)
        {
            std::vector<config_kit> kits;
            client.get_all_config_kits(kits);
            for (auto &kit : kits)
            {

                ret.insert({kit.kit_name, make_replay_url_or_cam_name(kit.config_items, _is_video)});
            }
        });
    return ret;
}
void al_record::vehicle_pass_record::generate_video()
{
    live_camera::call_live_camera_remote(
        [&](live_camera_serviceClient &client)
        {
            std::vector<video_download_progress> orig_list;
            client.get_video_download_progress(orig_list);
            client.generate_video(make_url_dev_map(true)[m_dev_name], m_begin_time, m_end_time);
            for (auto i = 0; i < 78; i++)
            {
                AD_RPC_SC::get_instance()->yield_by_timer(0, 260);
                std::vector<video_download_progress> new_list;
                client.get_video_download_progress(new_list);
                for (const auto &item : new_list)
                {
                    auto found_ret = std::find_if(
                        orig_list.begin(), orig_list.end(),
                        [&item](const video_download_progress &progress)
                        { return progress.name == item.name; });
                    if (found_ret == orig_list.end())
                    {
                        m_video_file_name = item.name;
                        break;
                    }
                }
                if (!m_video_file_name.empty())
                {
                    break;
                }
            }
        });
}
std::string al_record::vehicle_pass_record::make_live_url() const
{
    return make_url_dev_map()[m_dev_name] +
           "_start_" +
           al_utils::ad_utils_date_time::make_utc_time(m_begin_time) +
           "_end_" +
           al_utils::ad_utils_date_time::make_utc_time(m_end_time) + "/";
}
