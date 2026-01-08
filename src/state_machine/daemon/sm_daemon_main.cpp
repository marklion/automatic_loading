#include "state_machine_imp.h"
int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_SM_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<state_machine_serviceProcessor>(std::make_shared<state_machine_imp>()));
    sc->start_server();
    return 0;
}
