#if !defined(_AL_UTILS_H_)
#define _AL_UTILS_H_

#include <string>
#include <vector>
#include "../gen_code/cpp/public_idl_types.h"
#include "../gen_code/cpp/public_service.h"
#include "CJsonObject.hpp"

namespace al_utils
{
    std::vector<std::string> split_string(const std::string &str, char delimiter);
    std::string trim_string(const std::string &str);
    std::string join_strings(const std::vector<std::string> &strings, const std::string &delimiter);
    std::vector<std::shared_ptr<daemon_meta>> get_all_daemon_meta();
    std::string insert_spaces(const std::string &_str);
    struct ad_utils_date_time
    {
        std::string m_date;
        std::string m_time;
        std::string m_datetime_ms;
        std::string m_datetime;
        ad_utils_date_time(time_t now = time(nullptr))
        {
            struct tm tstruct;
            char buf[80];
            tstruct = *localtime(&now);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tstruct);
            // 获取毫秒
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            char ms_buf[10];
            snprintf(ms_buf, sizeof(ms_buf), ":%03ld", ts.tv_nsec / 1000000);

            m_datetime_ms = std::string(buf) + ms_buf;
            m_datetime = m_datetime_ms.substr(0, 19);
            m_date = m_datetime.substr(0, 10);
            m_time = m_datetime.substr(11, 8);
        }
    };
}

#endif // _AL_UTILS_H_
