#include "log_service_imp.h"
#include <iostream>
#include <fstream>
#include "../../config/lib/config_lib.h"
#include "../lib/log_lib.h"
class log_dispatch_data_node : public AD_EVENT_SC_TCP_DATA_NODE
{
    log_service_imp *m_service_imp;

public:
    using AD_EVENT_SC_TCP_DATA_NODE::AD_EVENT_SC_TCP_DATA_NODE;
    virtual void handleRead(const unsigned char *_data, unsigned long _size)
    {
    }
    virtual void handleError()
    {
        if (m_service_imp)
        {
            m_service_imp->remove_data_node(std::static_pointer_cast<AD_EVENT_SC_TCP_DATA_NODE>(shared_from_this()));
        }
    };
    void set_service_imp(log_service_imp *_imp)
    {
        m_service_imp = _imp;
    }
};
log_service_imp::log_service_imp()
{
    m_listen_node.reset(
        std::make_unique<AD_EVENT_SC_TCP_LISTEN_NODE>(
            AD_BUSINESS_LOG_SERVER_PORT,
            [this](int _fd, AD_EVENT_SC_TCP_LISTEN_NODE_PTR _listen_node)
            {
                auto data_node = std::make_shared<log_dispatch_data_node>(_fd, _listen_node);
                m_data_nodes.push_back(data_node);
                data_node->set_service_imp(this);
                return data_node;
            },
            AD_RPC_SC::get_instance())
            .release());
    AD_RPC_SC::get_instance()->registerNode(m_listen_node);
}
log_service_imp::~log_service_imp()
{
    AD_RPC_SC::get_instance()->unregisterNode(m_listen_node);
}

void log_service_imp::log_print(const std::string &log_msg, const int32_t module_id, const int32_t level_id)
{
    std::ofstream ofs;
    std::string file_base = "/database/";
    std::string file_name = config::root_config::get_instance()(CONFIG_ITEM_LOG_FILE);
    if (file_name.empty())
    {
        file_name = "default.log";
    }
    if (level_id == al_log::LOG_LEVEL_ERROR)
    {
        ofs.open(file_base + file_name, std::ios::out | std::ios::app);
        if (ofs.is_open())
        {
            ofs << log_msg;
            ofs.close();
            // Check file size and trim if necessary
            std::ifstream check_file(file_base + file_name, std::ios::binary | std::ios::ate);
            if (check_file.is_open())
            {
                if (check_file.tellg() > 100000)
                {
                    check_file.close();
                    // Read all lines and skip first 100
                    std::ifstream input_file(file_base + file_name);
                    std::vector<std::string> lines;
                    std::string line;
                    int line_count = 0;
                    while (std::getline(input_file, line))
                    {
                        if (line_count >= 100)
                        {
                            lines.push_back(line);
                        }
                        line_count++;
                    }
                    input_file.close();

                    // Write back
                    std::ofstream output_file(file_base + file_name);
                    for (const auto &l : lines)
                    {
                        output_file << l << "\n";
                    }
                    output_file.close();
                }
                else
                {
                    check_file.close();
                }
            }
        }
    }

    if (should_log((al_log::LOG_MODULE)module_id, (al_log::LOG_LEVEL)level_id))
    {
        dispatch_log_message(log_msg);
    }
}

bool log_service_imp::set_log_file(const std::string &file_path)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_LOG_FILE, file_path);
    return true;
}

void log_service_imp::get_log_file(std::string &_return)
{
    auto &ci = config::root_config::get_instance();
    auto ret = ci(CONFIG_ITEM_LOG_FILE);
    if (ret.empty())
    {
        ret = "default.log";
    }
    _return = ret;
}

void log_service_imp::unset_log_level(const int32_t module_id)
{
    auto &ci = config::root_config::get_instance();
    auto module_str = al_log::LOG_MODULE_NAME[(al_log::LOG_MODULE)module_id];
    if (module_str.length() > 0)
    {
        ci.remove_child(module_str + CONFIG_ITEM_LOG_MODULE);
    }
    else
    {
        for (const auto &pair : al_log::LOG_MODULE_NAME)
        {
            ci.remove_child(pair.second + CONFIG_ITEM_LOG_MODULE);
        }
    }
}

void log_service_imp::get_log_level(std::vector<log_level_info> &_return)
{
    auto &ci = config::root_config::get_instance();
    for (const auto &pair : al_log::LOG_MODULE_NAME)
    {
        auto level_str = ci(pair.second + CONFIG_ITEM_LOG_MODULE);
        if (level_str.length() > 0)
        {
            log_level_info level_info;
            level_info.module_id = (int32_t)pair.first;
            if (level_str == "DEBUG")
            {
                level_info.level_id = 0;
            }
            else if (level_str == "INFO")
            {
                level_info.level_id = 1;
            }
            else if (level_str == "WARN")
            {
                level_info.level_id = 2;
            }
            else if (level_str == "ERROR")
            {
                level_info.level_id = 3;
            }
            _return.push_back(level_info);
        }
    }
}

static bool log_match_req(const std::string &_line, const log_level_info &level_info)
{
    bool ret = false;
    if (_line.find(al_log::LOG_MODULE_NAME[(al_log::LOG_MODULE)level_info.module_id]) != std::string::npos)
    {
        switch (level_info.level_id)
        {
        case 0:
            if (_line.find("DEBUG") != std::string::npos)
            {
                ret = true;
            }
            break;
        case 1:
            if (_line.find("INFO") != std::string::npos)
            {
                ret = true;
            }
            break;
        case 2:
            if (_line.find("WARN") != std::string::npos)
            {
                ret = true;
            }
            break;
        case 3:
            if (_line.find("ERROR") != std::string::npos)
            {
                ret = true;
            }
            break;
        default:
            ret = true;
            break;
        }
    }
    return ret;
}

void log_service_imp::get_lastest_logs(std::vector<std::string> &_return, const log_level_info &level_info, const int32_t last_line_index)
{
    std::string file_name;
    get_log_file(file_name);
    auto full_path = "/database/" + file_name;
    std::ifstream ifs(full_path);
    if (ifs.is_open())
    {
        std::string line;
        int32_t current_index = 0;
        while (std::getline(ifs, line))
        {
            if (log_match_req(line, level_info) == false)
            {
                continue;
            }
            if (current_index >= last_line_index)
            {
                _return.push_back(line);
            }
            current_index++;
        }
        ifs.close();
    }
}

int32_t log_service_imp::get_log_cur_line_num(const log_level_info &level_info)
{
    int32_t ret = 0;
    std::string file_name;
    get_log_file(file_name);
    auto full_path = "/database/" + file_name;
    std::ifstream ifs(full_path);
    if (ifs.is_open())
    {
        std::string line;
        int32_t current_index = 0;
        while (std::getline(ifs, line))
        {
            if (log_match_req(line, level_info) == false)
            {
                continue;
            }
            current_index++;
        }
        ifs.close();
        ret = current_index;
    }
    return ret;
}

bool log_service_imp::should_log(al_log::LOG_MODULE _module, al_log::LOG_LEVEL _level)
{
    bool ret = false;
    auto &ci = config::root_config::get_instance();
    auto module_str = al_log::LOG_MODULE_NAME[_module];
    auto log_level_str = ci(module_str + CONFIG_ITEM_LOG_MODULE);
    al_log::LOG_LEVEL config_level = al_log::LOG_LEVEL_NONE;
    if (log_level_str == "DEBUG")
    {
        config_level = al_log::LOG_LEVEL_DEBUG;
    }
    else if (log_level_str == "INFO")
    {
        config_level = al_log::LOG_LEVEL_INFO;
    }
    else if (log_level_str == "WARN")
    {
        config_level = al_log::LOG_LEVEL_WARN;
    }
    else if (log_level_str == "ERROR")
    {
        config_level = al_log::LOG_LEVEL_ERROR;
    }
    if (_level >= config_level)
    {
        ret = true;
    }
    return ret;
}

void log_service_imp::set_log_level(const log_level_info &level_info)
{
    auto &ci = config::root_config::get_instance();
    std::string level_str;
    switch (level_info.level_id)
    {
    case 0:
        level_str = "DEBUG";
        break;
    case 1:
        level_str = "INFO";
        break;
    case 2:
        level_str = "WARN";
        break;
    case 3:
        level_str = "ERROR";
        break;
    }
    auto module_str = al_log::LOG_MODULE_NAME[(al_log::LOG_MODULE)level_info.module_id];
    if (module_str.length() > 0 && level_str.length() > 0)
    {
        ci.remove_child(module_str + CONFIG_ITEM_LOG_MODULE);
        ci.set_child(module_str + CONFIG_ITEM_LOG_MODULE, level_str);
    }
}

void log_service_imp::dispatch_log_message(const std::string &log_msg)
{
    for (auto &node : m_data_nodes)
    {
        send(node->getFd(), log_msg.c_str(), log_msg.size(), SOCK_NONBLOCK);
    }
}
