#include "al_utils.h"
#include "ad_rpc.h"
#include <sstream>
#include <iconv.h>
#include <fstream>
#include "CJsonObject.hpp"
#include <algorithm>

#define AL_USER_INFO_FILE "/database/users.json"

namespace al_utils
{
    std::string g_self_module_name;
    bool g_exception_happened = false;
    static int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen)
    {
        iconv_t cd;
        int rc;
        char **pin = &inbuf;
        char **pout = &outbuf;

        cd = iconv_open(to_charset, from_charset);
        if (cd == 0)
            return -1;
        memset(outbuf, 0, outlen);
        if (iconv(cd, pin, &inlen, pout, &outlen) == -1)
            return -1;
        iconv_close(cd);
        return 0;
    }
    // UNICODE码转为GB2312码
    static int u2g(char *inbuf, int inlen, char *outbuf, int outlen)
    {
        return code_convert("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
    }
    static int g2u(char *inbuf, int inlen, char *outbuf, int outlen)
    {
        return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
    }
    std::string util_gbk2utf(const std::string &_utf)
    {
        char in_buff[9600] = {0};
        char out_buff[9600] = {0};
        strcpy(in_buff, _utf.c_str());
        g2u(in_buff, strlen(in_buff), out_buff, sizeof(out_buff));
        return std::string(out_buff);
    }
    std::string double2string(const double _value, const int _precision)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(_precision) << _value;
        return out.str();
    }
    long long get_current_us_stamp()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        long long current_useconds = tv.tv_sec * 1000000LL + tv.tv_usec;
        return current_useconds;
    }
    void start_server_notify_started(const std::string &module_name)
    {
        AD_RPC_SC::get_instance()->start_server(
            [module_name]()
            {
                AD_RPC_SC::get_instance()->call_remote<public_serviceClient>(
                    AD_RPC_PROCESS_SERVER_PORT,
                    [module_name](public_serviceClient &client)
                    {
                        client.notify_started(module_name);
                    });
                g_self_module_name = module_name;
            });
    }
    void record_self_health(const std::string &_except_info)
    {
        bool should_record = false;
        if (_except_info.empty())
        {
            if (g_exception_happened)
            {
                should_record = true;
                g_exception_happened = false;
            }
        }
        else
        {
            if (!g_exception_happened)
            {
                should_record = true;
                g_exception_happened = true;
            }
        }
        if (should_record)
        {
            AD_RPC_SC::get_instance()->call_remote<public_serviceClient>(
                AD_RPC_PROCESS_SERVER_PORT,
                [&](public_serviceClient &client)
                {
                    client.record_health(_except_info, g_self_module_name);
                });
        }
    }
    void get_health_records(std::vector<health_info> &_return)
    {
        AD_RPC_SC::get_instance()->call_remote<public_serviceClient>(
            AD_RPC_PROCESS_SERVER_PORT,
            [&](public_serviceClient &client)
            {
                client.get_health_records(_return);
            });
    }
    void set_watch_dog_param(const watch_dog_info &info, const bool is_clear)
    {
        AD_RPC_SC::get_instance()->call_remote<public_serviceClient>(
            AD_RPC_PROCESS_SERVER_PORT,
            [&](public_serviceClient &client)
            {
                client.set_watch_dog_param(info, is_clear);
            });
    }
    watch_dog_info get_watch_dog_param()
    {
        watch_dog_info ret;
        AD_RPC_SC::get_instance()->call_remote<public_serviceClient>(
            AD_RPC_PROCESS_SERVER_PORT,
            [&](public_serviceClient &client)
            {
                client.get_watch_dog_param(ret);
            });
        return ret;
    }
    void active_watch_dog(bool is_active)
    {
        AD_RPC_SC::get_instance()->call_remote<public_serviceClient>(
            AD_RPC_PROCESS_SERVER_PORT,
            [&](public_serviceClient &client)
            {
                if (is_active)
                {
                    client.active_watch_dog();
                }
                else
                {
                    client.reset_watch_dog();
                }
            });
    }
    void update_all_user(const std::vector<al_user_info> &_users)
    {
        std::ofstream ofs(AL_USER_INFO_FILE, std::ios::out | std::ios::trunc);
        if (ofs.is_open())
        {
            neb::CJsonObject json_array("[]");
            for (const auto &user : _users)
            {
                neb::CJsonObject json_user;
                json_user.Add("username", user.username);
                json_user.Add("password", user.password);
                json_array.Add(json_user);
            }
            ofs << json_array.ToString() << std::endl;
            ofs.close();
        }

    }
    void add_user(const std::string &_username, const std::string &_password)
    {
        auto users = list_users();
        auto exist_user_itr = std::find_if(
            users.begin(),
            users.end(),
            [&_username](const al_user_info &user)
            {
                return user.username == _username;
            });
        if (exist_user_itr != users.end())
        {
            exist_user_itr->password = _password;
        }
        else
        {
            users.push_back({_username, _password});
        }
        update_all_user(users);
    }
    void del_user(const std::string &_username)
    {
        auto users = list_users();
        auto exist_user_itr = std::find_if(
            users.begin(),
            users.end(),
            [&_username](const al_user_info &user)
            {
                return user.username == _username;
            });
        if (exist_user_itr != users.end())
        {
            users.erase(exist_user_itr);
            update_all_user(users);
        }
    }
    std::vector<al_user_info> list_users()
    {
        std::vector<al_user_info> ret;
        std::ifstream ifs(AL_USER_INFO_FILE);
        if (ifs.is_open())
        {
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
            neb::CJsonObject json_array(content);
            for (size_t i = 0; i < json_array.GetArraySize(); ++i)
            {
                neb::CJsonObject json_user;
                json_array.Get(i, json_user);
                al_user_info user;
                json_user.Get("username", user.username);
                json_user.Get("password", user.password);
                ret.push_back(user);
            }
        }

        return ret;
    }
    bool verify_user(const std::string &_username, const std::string &_password)
    {
        auto users = list_users();
        auto exist_user_itr = std::find_if(
            users.begin(),
            users.end(),
            [&_username](const al_user_info &user)
            {
                return user.username == _username;
            });
        if (exist_user_itr != users.end())
        {
            return exist_user_itr->password == _password;
        }
        else
        {
            return false;
        }
    }
    void clear_users()
    {
        std::ofstream ofs(AL_USER_INFO_FILE, std::ios::out | std::ios::trunc);
        if (ofs.is_open())
        {
            ofs << "[]" << std::endl;
            ofs.close();
        }
    }
    std::string util_utf2gbk(const std::string &_gbk)
    {
        char in_buff[9600] = {0};
        char out_buff[9600] = {0};
        strcpy(in_buff, _gbk.c_str());
        u2g(in_buff, strlen(in_buff), out_buff, sizeof(out_buff));
        return std::string(out_buff);
    }
    std::vector<std::string> split_string(const std::string &str, char delimiter)
    {
        std::vector<std::string> result;
        std::string current;
        for (char ch : str)
        {
            if (ch == delimiter)
            {
                if (!current.empty())
                {
                    result.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current += ch;
            }
        }
        if (!current.empty())
        {
            result.push_back(current);
        }
        return result;
    }

    std::string trim_string(const std::string &str)
    {
        const char *whitespace = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(whitespace);
        size_t end = str.find_last_not_of(whitespace);
        return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }

    std::string join_strings(const std::vector<std::string> &strings, const std::string &delimiter)
    {
        std::string result;
        for (size_t i = 0; i < strings.size(); ++i)
        {
            result += strings[i];
            if (i < strings.size() - 1)
            {
                result += delimiter;
            }
        }
        return result;
    }
    std::vector<std::shared_ptr<daemon_meta>> get_all_daemon_meta()
    {
        std::vector<std::shared_ptr<daemon_meta>> ret;
        AD_RPC_SC::get_instance()->call_remote<public_serviceClient>(
            AD_RPC_PROCESS_SERVER_PORT,
            [&ret](public_serviceClient &client)
            {
                std::vector<daemon_meta> metas;
                client.get_all_daemon_meta(metas);
                for (auto &item : metas)
                {
                    ret.push_back(std::make_shared<daemon_meta>(item));
                }
            });

        return ret;
    }
    std::string insert_spaces(const std::string &_str)
    {
        std::stringstream ss(_str);
        std::string line;
        std::string result;
        while (std::getline(ss, line))
        {
            result += "  " + line + "\n";
        }
        return result;
    }

    std::string get_current_timestamp_ms()
    {
        std::string ret;

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        long long milliseconds = static_cast<long long>(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
        ret = std::to_string(milliseconds);

        return ret;
    }
    std::string ad_utils_date_time::date_plus_day(const std::string &_date, int _day)
    {
        std::string ret;

        struct tm tstruct = {0};
        strptime(_date.c_str(), "%Y-%m-%d", &tstruct);
        time_t time = mktime(&tstruct);
        time += _day * 24 * 3600;
        struct tm new_tstruct = {0};
        localtime_r(&time, &new_tstruct);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d", &new_tstruct);
        ret = std::string(buf);

        return ret;
    }
    bool ad_utils_date_time::is_before(const std::string &datetime1, const std::string &datetime2)
    {
        bool ret = false;

        struct tm tstruct1 = {0};
        struct tm tstruct2 = {0};
        strptime(datetime1.c_str(), "%Y-%m-%d %H:%M:%S", &tstruct1);
        strptime(datetime2.c_str(), "%Y-%m-%d %H:%M:%S", &tstruct2);
        time_t time1 = mktime(&tstruct1);
        time_t time2 = mktime(&tstruct2);
        if (difftime(time1, time2) < 0)
        {
            ret = true;
        }

        return ret;
    }
    std::string ad_utils_date_time::make_utc_time(const std::string &datetime)
    {
        std::string ret;

        char buf[80];
        struct tm tstruct = {0};
        strptime(datetime.c_str(), "%Y-%m-%d %H:%M:%S", &tstruct);

        strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tstruct);
        ret = std::string(buf);

        return ret;
    }
}