#if !defined(_RECORD_LIB_H_)
#define _RECORD_LIB_H_
#include <string>
#include <vector>
#include "../../live_camera/lib/live_camera_lib.h"

namespace al_record
{
    struct vehicle_pass_record
    {
        std::string m_plate;
        std::string m_begin_time;
        std::string m_end_time;
        std::string m_dev_name;
        std::string m_video_file_name;
        int m_video_download_progress = 0;
        double m_load = 0;
        bool m_is_json = false;
        bool m_justified = false;
        vehicle_pass_record(
            const std::string &plate,
            const std::string &begin_time,
            const std::string &end_time,
            const std::string &dev_name,
            double _load,
            bool _justified):
            m_plate(plate),
            m_begin_time(begin_time),
            m_end_time(end_time),
            m_dev_name(dev_name),
            m_load(_load),
            m_justified(_justified)
        {
        }
        vehicle_pass_record() {}
        void generate_video();
        std::string make_live_url() const;

    };
    void record_vehicle_pass(const vehicle_pass_record &record);
    void search_vp_list(std::vector<vehicle_pass_record> &_return, const std::string &_plate, const std::string &_begin_time, const std::string &_end_time, const std::string &_dev_name);
    void refresh_vp(const vehicle_pass_record &record);
    int get_progress_by_file_name(const std::string &file_name, const std::vector<video_download_progress> &_vdp);

} // namespace al_record


#endif // _RECORD_LIB_H_
