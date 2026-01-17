#if !defined(_HN_HHT_LIB_H_)
#define _HN_HHT_LIB_H_

#include "../gen_code/cpp/hn_hht_idl_types.h"
#include "../gen_code/cpp/hn_hht_service.h"
#include "../../public/lib/ad_rpc.h"


namespace hn_hht
{
    void call_hht_service_remote(std::function<void(hn_hht_serviceClient&)> func);
    std::string req_get_order(const std::string& query_content);
};

#endif // _HN_HHT_LIB_H_
