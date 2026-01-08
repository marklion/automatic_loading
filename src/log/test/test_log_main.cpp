#include "../lib/log_lib.h"
#include "../../public/lib/ad_rpc.h"
int main(int argc, char const *argv[])
{
    auto log_content = argv[1];
    auto sc = AD_RPC_SC::get_instance();
    sc->startTimer(
        1,
        [&]() {
            al_log::log_tool(al_log::LOG_TEST).log_print(al_log::LOG_LEVEL_WARN, "%s", log_content);
            sc->stop_server();
        });
    sc->start_server();
    return 0;
}
