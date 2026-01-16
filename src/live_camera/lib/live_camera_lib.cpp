#include "live_camera_lib.h"
#include "../../public/lib/ad_rpc.h"
void live_camera::call_live_camera_remote(std::function<void(live_camera_serviceClient &)> func)
{
    AD_RPC_SC::get_instance()->call_remote<live_camera_serviceClient>(AD_RPC_LIVE_STREAM_SERVER_PORT, func);
}
