#include "scale_imp.h"
#include "../../public/lib/al_utils.h"

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    auto psi = std::make_shared<scale_main_impl>();
    sc->enable_rpc_server(AD_RPC_SCALE_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<scale_serviceProcessor>(psi));
    sc->startTimer(
        3, [&]()
        {
            if (psi->is_offline())
            {
                al_utils::record_self_health("scale offline");
            }
            else
            {
                al_utils::record_self_health("");
            }
        });
    al_utils::start_server_notify_started("scale");
    return 0;
}
