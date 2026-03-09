#if !defined(_DS_LIB_H_)
#define _DS_LIB_H_

#include "../gen_code/cpp/drop_system_service.h"
#include "../gen_code/cpp/drop_system_idl_types.h"
#include "../../public/lib/ad_rpc.h"

namespace drop_system
{
    void call_remote_ds(std::function<void(drop_system_serviceClient &)> _func);
};

#endif // _DS_LIB_H_
