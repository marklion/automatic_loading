#include "record_lib.h"
#include <memory>
#include <fstream>
#include <dirent.h>
#include "../../public/lib/al_utils.h"

#define VP_FILE_NAME_PREFIX "vehicle_pass_record_"
#define RECORD_FILE_PATH_PREFIX "/database/" VP_FILE_NAME_PREFIX

static std::string make_one_line_record(const al_record::vehicle_pass_record &record)
{
    return record.m_plate + "," + record.m_begin_time + "," + record.m_end_time + "," + record.m_dev_name;
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
    std::string plate = line.substr(0, pos1);
    std::string begin_time = line.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string end_time = line.substr(pos2 + 1, pos3 - pos2 - 1);
    std::string dev_name = line.substr(pos3 + 1);
    return std::make_unique<al_record::vehicle_pass_record>(plate, begin_time, end_time, dev_name);
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
    std::vector<std::string> file_list;
    search_file_list(file_list, _begin_time, _end_time);
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
                        _return.push_back(record);
                    }
                }
            }
        }
    }
}
