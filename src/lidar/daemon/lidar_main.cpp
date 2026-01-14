#include "lidar_imp.h"
int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_LIDAR_SERVER_PORT);
    auto li = std::make_shared<lidar_imp>();
    sc->add_rpc_server(std::make_shared<lidar_serviceProcessor>(li));
    sc->add_co(
        [li]()
        {
            li->start_all_lidar_threads();
        });
    sc->start_server();
    return 0;
}