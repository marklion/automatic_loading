#include "xlrd_lib.h"
#include "../../public/lib/ad_rpc.h"

void ad_xlrd::call_xlrd_service_remote(std::function<void(xlrd_serviceClient &)> func)
{
    AD_RPC_SC::get_instance()->call_remote(
        AD_RPC_XLRD_SERVER_PORT,
        func);
}
