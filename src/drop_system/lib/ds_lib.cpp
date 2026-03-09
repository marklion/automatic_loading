#include "ds_lib.h"

void drop_system::call_remote_ds(std::function<void(drop_system_serviceClient &)> _func)
{
    AD_RPC_SC::get_instance()->call_remote<drop_system_serviceClient>(
        AD_RPC_DROP_SYSTEM_SERVER_PORT,
        [&](drop_system_serviceClient &_client)
        {
            _func(_client);
        });
}