#include "lidar_imp.h"
#include "../../public/lib/al_utils.h"
int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_LIDAR_SERVER_PORT);
    auto li = std::make_shared<lidar_imp>();
    sc->add_rpc_server(std::make_shared<lidar_serviceProcessor>(li));
    al_utils::start_server_notify_started("lidar");
    return 0;
}