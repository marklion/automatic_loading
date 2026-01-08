#include "../lib/log_lib.h"
#include "../../public/lib/ad_rpc.h"
#include <thrift/TProcessor.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/processor/TMultiplexedProcessor.h>
#include <thrift/protocol/TMultiplexedProtocol.h>
#include "../gen_code/cpp/log_idl_types.h"
#include "../gen_code/cpp/log_service.h"
#include "log_service_imp.h"

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_LOG_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<log_serviceProcessor>(std::make_shared<log_service_imp>()));
    sc->start_server();
    return 0;
}
