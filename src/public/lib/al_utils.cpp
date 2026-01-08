#include "al_utils.h"
#include "ad_rpc.h"
namespace al_utils
{
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
}