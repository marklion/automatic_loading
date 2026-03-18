#include "modbus_io_lib.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"

void modbus_io::set_one_io(const std::string &_name, bool _is_set,const std::string &_control_source)
{
    al_log::log_tool logger(al_log::LOG_MODBUS_IO);
    logger.log_print(al_log::LOG_LEVEL_INFO, "Set io [%s] to [%s] by [%s]", _name.c_str(), _is_set ? "ON" : "OFF", _control_source.c_str());
    AD_RPC_SC::get_instance()->call_remote<modbus_io_serviceClient>(
        AD_RPC_MODBUS_IO_SERVER_PORT,
        [&](modbus_io_serviceClient &client)
        {
            client.device_io_set(_name, _is_set);
        });
}

bool modbus_io::get_one_io(const std::string &_name)
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
