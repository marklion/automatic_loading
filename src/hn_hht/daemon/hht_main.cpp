#include "../gen_code/cpp/hn_hht_idl_types.h"
#include "../gen_code/cpp/hn_hht_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../public/lib/al_utils.h"
#include "../../public/lib/CJsonObject.hpp"
#include "../../log/lib/log_lib.h"
#include "../../config/lib/config_lib.h"
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <mutex>

static al_log::log_tool g_logger(al_log::LOG_HHT);
static std::unique_ptr<AD_CO_MUTEX> g_mutex = AD_RPC_SC::get_instance()->create_co_mutex();
class hn_hht_imp : public hn_hht_serviceIf
{
public:
    virtual bool set_params(const hht_config_params &_params)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_HHT_KEY, _params.app_key);
        ci.set_child(CONFIG_ITEM_HHT_SECRET, _params.app_secret);
        return true;
    }
    virtual void get_params(hht_config_params &_return)
    {
        auto &ci = config::root_config::get_instance();
        _return.app_key = ci(CONFIG_ITEM_HHT_KEY);
        _return.app_secret = ci(CONFIG_ITEM_HHT_SECRET);
    }
    virtual void get_order(std::string &_return, const std::string &_query_content)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        auto &ci = config::root_config::get_instance();
        std::string app_key = ci(CONFIG_ITEM_HHT_KEY);
        std::string app_secret = ci(CONFIG_ITEM_HHT_SECRET);
        std::string timestamp = al_utils::get_current_timestamp_ms();
        auto content = al_utils::URLCodec::encode_query_param(_query_content);
        auto sig_cmd = "/root/sm4_tool/run_signature.sh '" + app_key + "' '" + app_secret + "' '" + timestamp + "' | awk -F ': ' '{print $2}' > /tmp/hht_sig_resp.txt";
        AD_RPC_SC::get_instance()->non_block_system(sig_cmd);
        std::ifstream sig_ifs("/tmp/hht_sig_resp.txt");
        std::string signature;
        if (sig_ifs.is_open())
        {
            std::getline(sig_ifs, signature);
            sig_ifs.close();
        }
        std::string get_req = "https://openapi.hnjt.top/api/openapi/v1/transport/billInfo?no=" + content;
        std::string wget_cmd = "wget --no-check-certificate --method GET --timeout=15 ";
        wget_cmd += "--header 'appKey: " + app_key + "' ";
        wget_cmd += "--header 'signature: " + signature + "' ";
        wget_cmd += "--header 'timestamp: " + timestamp + "' ";
        wget_cmd += "'" + get_req + "' -O /tmp/hht_order_response.json 2> /tmp/hht_log.txt";
        g_logger.log_print(al_log::LOG_LEVEL_INFO, "cmd length:%d", wget_cmd.length());
        g_logger.log_print(al_log::LOG_LEVEL_INFO, "Generated signature: %s", signature.c_str());
        g_logger.log_print(al_log::LOG_LEVEL_INFO, "Executing wget command: %s", wget_cmd.c_str());
        AD_RPC_SC::get_instance()->non_block_system(wget_cmd);
        std::ifstream log_ifs("/tmp/hht_log.txt");
        if (log_ifs.is_open())
        {
            std::stringstream log_buffer;
            log_buffer << log_ifs.rdbuf();
            g_logger.log_print(al_log::LOG_LEVEL_INFO, "Wget log: %s", log_buffer.str().c_str());
            log_ifs.close();
        }
        std::ifstream ifs("/tmp/hht_order_response.json");
        if (ifs.is_open())
        {
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            _return = buffer.str();
            ifs.close();
        }
        else
        {
            g_logger.log_print(al_log::LOG_LEVEL_ERROR, "Failed to open HHT order response file");
            _return = "";
        }
    }
};

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_HHT_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<hn_hht_serviceProcessor>(std::make_shared<hn_hht_imp>()));
    sc->start_server();
    return 0;
}
