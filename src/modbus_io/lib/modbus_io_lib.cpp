#include "modbus_io_lib.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"

void modbus_io::set_one_io(std::string &_name, bool _is_set)
{
    AD_RPC_SC::get_instance()->call_remote<modbus_io_serviceClient>(
        AD_RPC_MODBUS_IO_SERVER_PORT,
        [&](modbus_io_serviceClient &client)
        {
            client.device_io_set(_name, _is_set);
        });
}

bool modbus_io::get_one_io(std::string &_name)
{
    bool ret = false;
    AD_RPC_SC::get_instance()->call_remote<modbus_io_serviceClient>(
        AD_RPC_MODBUS_IO_SERVER_PORT,
        [&](modbus_io_serviceClient &client)
        {
            ret = client.device_io_get(_name);
        });
    return ret;
}

void modbus_io::call_remote_modbus_service(std::function<void(modbus_io_serviceClient &)> _func)
{
    AD_RPC_SC::get_instance()->call_remote<modbus_io_serviceClient>(
        AD_RPC_MODBUS_IO_SERVER_PORT,
        _func);
}
