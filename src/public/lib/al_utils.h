#if !defined(_AL_UTILS_H_)
#define _AL_UTILS_H_

#include <string>
#include <vector>
#include "../gen_code/cpp/public_idl_types.h"
#include "../gen_code/cpp/public_service.h"
#include "CJsonObject.hpp"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>

namespace al_utils
{
    std::vector<std::string> split_string(const std::string &str, char delimiter);
    std::string trim_string(const std::string &str);
    std::string join_strings(const std::vector<std::string> &strings, const std::string &delimiter);
    std::vector<std::shared_ptr<daemon_meta>> get_all_daemon_meta();
    std::string insert_spaces(const std::string &_str);
    std::string get_current_timestamp_ms();
    std::string util_gbk2utf(const std::string &_utf);
    std::string double2string(const double _value, const int _precision = 2);
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
    class URLCodec
    {
    public:
        // URL 编码
        static std::string encode(const std::string &str, bool space_as_plus = true)
        {
            std::ostringstream encoded;
            encoded << std::hex << std::uppercase;

            for (unsigned char c : str)
            {
                if (should_encode(c))
                {
                    encoded << c;
                }
                else if (c == ' ' && space_as_plus)
                {
                    encoded << '+';
                }
                else
                {
                    encoded << '%' << std::setw(2) << std::setfill('0')
                            << static_cast<int>(c);
                }
            }

            return encoded.str();
        }

        // URL 解码
        static std::string decode(const std::string &str)
        {
            std::ostringstream decoded;

            for (size_t i = 0; i < str.length(); i++)
            {
                if (str[i] == '%' && i + 2 < str.length())
                {
                    // 处理百分号编码
                    std::string hex = str.substr(i + 1, 2);
                    char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                    decoded << decoded_char;
                    i += 2;
                }
                else if (str[i] == '+')
                {
                    // 加号转换为空格
                    decoded << ' ';
                }
                else
                {
                    decoded << str[i];
                }
            }

            return decoded.str();
        }

        // 查询字符串编码（特殊处理 & 和 =）
        static std::string encode_query_param(const std::string &str)
        {
            std::ostringstream encoded;
            encoded << std::hex << std::uppercase;

            for (unsigned char c : str)
            {
                if (std::isalnum(c) ||
                    c == '-' || c == '_' || c == '.' || c == '~')
                {
                    encoded << c;
                }
                else
                {
                    encoded << '%' << std::setw(2) << std::setfill('0')
                            << static_cast<int>(c);
                }
            }

            return encoded.str();
        }

    private:
        static bool should_encode(unsigned char c)
        {
            // RFC 3986 安全字符
            static const std::string safe_chars =
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "0123456789"
                "-_.~";

            return std::isalnum(c) ||
                   c == '-' || c == '_' || c == '.' || c == '~' ||
                   c == '!' || c == '*' || c == '\'' || c == '(' || c == ')' ||
                   c == ';' || c == '&' ||
                   c == '=' || c == '+' || c == '$' || c == ',' ||
                   c == '/' || c == '?' || c == '#' || c == '[' || c == ']';
        }
    };
}

#endif // _AL_UTILS_H_
