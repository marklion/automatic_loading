#if !defined(_STATE_MACHINE_LIB_H_)
#define _STATE_MACHINE_LIB_H_

#include "../gen_code/cpp/state_machine_service.h"
#include "../gen_code/cpp/state_machine_idl_types.h"
#include "../../public/lib/ad_rpc.h"

namespace state_machine
{
    void call_sm_remote(std::function<void(state_machine_serviceClient &)> func);
}


#endif // _STATE_MACHINE_LIB_H_
