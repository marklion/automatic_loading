#include "scale_lib.h"

void al_scale::call_scale_service(std::function<void(scale_serviceClient &)> _func)
{
    AD_RPC_SC::get_instance()->call_remote<scale_serviceClient>(
        AD_RPC_SCALE_SERVER_PORT,
        _func);
}