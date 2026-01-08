#include "state_machine_lib.h"

void state_machine::call_sm_remote(std::function<void(state_machine_serviceClient &)> func)
{
    AD_RPC_SC::get_instance()->call_remote(
        AD_RPC_SM_SERVER_PORT,
        func
    );
}
