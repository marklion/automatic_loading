#include "scale_imp.h"

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    auto psi = std::make_shared<scale_main_impl>();
    sc->enable_rpc_server(AD_RPC_SCALE_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<scale_serviceProcessor>(psi));
    sc->start_server();
    return 0;
}
