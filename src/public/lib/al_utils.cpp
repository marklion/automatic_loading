#include "al_utils.h"
#include "ad_rpc.h"
#include <sstream>
#include <iconv.h>
namespace al_utils
{
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
}