#if !defined(_LOG_SERVICE_H_)
#define _LOG_SERVICE_H_

#include "../gen_code/cpp/log_idl_types.h"
#include "../gen_code/cpp/log_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../lib/log_lib.h"

class log_service_imp : public log_serviceIf
{
    AD_EVENT_SC_TCP_LISTEN_NODE_PTR m_listen_node;
    std::vector<AD_EVENT_SC_TCP_DATA_NODE_PTR> m_data_nodes;

public:
    log_service_imp();
    virtual ~log_service_imp();
    virtual void log_print(const std::string &log_msg, const int32_t module_id, const int32_t level_id);
    virtual bool set_log_file(const std::string &file_path);
    virtual void get_log_file(std::string &_return);
    virtual void set_log_level(const log_level_info &level_info);
    virtual void unset_log_level(const int32_t module_id);
    virtual void get_log_level(std::vector<log_level_info> &_return);
    virtual void get_lastest_logs(std::vector<std::string> &_return, const log_level_info &level_info, const int32_t last_line_index);
    virtual int32_t get_log_cur_line_num(const log_level_info &level_info);
    bool should_log(al_log::LOG_MODULE _module, al_log::LOG_LEVEL _level);
    void remove_data_node(AD_EVENT_SC_TCP_DATA_NODE_PTR _node)
    {
        auto itr = std::find(m_data_nodes.begin(), m_data_nodes.end(), _node);
        if (itr != m_data_nodes.end())
        {
            m_data_nodes.erase(itr);
        }
    }
    void dispatch_log_message(const std::string &log_msg);
};

#endif // _LOG_SERVICE_H_
