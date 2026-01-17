#include "hn_hht_lib.h"

void hn_hht::call_hht_service_remote(std::function<void(hn_hht_serviceClient &)> func)
{
    AD_RPC_SC::get_instance()->call_remote<hn_hht_serviceClient>(
        AD_RPC_HHT_SERVER_PORT,
        func);
}

std::string hn_hht::req_get_order(const std::string &query_content)
{
    std::string ret;

    hn_hht::call_hht_service_remote(
        [&](hn_hht_serviceClient &client)
        {
            client.get_order(ret, query_content);
        });

    return ret;
}
