#include "lidar_lib.h"

void ad_lidar::call_sm_remote(std::function<void(lidar_serviceClient &)> func)
{
    AD_RPC_SC::get_instance()->call_remote(
        AD_RPC_LIDAR_SERVER_PORT,
        func);
}
