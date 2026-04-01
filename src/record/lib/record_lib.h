#if !defined(_RECORD_LIB_H_)
#define _RECORD_LIB_H_
#include <string>
#include <vector>

namespace al_record
{
    struct vehicle_pass_record
    {
        std::string m_plate;
        std::string m_begin_time;
        std::string m_end_time;
        std::string m_dev_name;
        double m_load = 0;
        bool m_is_json = false;
        vehicle_pass_record(const std::string &plate, const std::string &begin_time, const std::string &end_time, const std::string &dev_name, double _load) : m_plate(plate), m_begin_time(begin_time), m_end_time(end_time), m_dev_name(dev_name),m_load(_load)
        {
        }
        vehicle_pass_record() {}
    };
    void record_vehicle_pass(const vehicle_pass_record &record);
    void search_vp_list(std::vector<vehicle_pass_record> &_return, const std::string &_plate, const std::string &_begin_time, const std::string &_end_time, const std::string &_dev_name);
} // namespace al_record


#endif // _RECORD_LIB_H_
