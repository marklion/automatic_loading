#if !defined(_MODBUS_IO_LIB_H_)
#define _MODBUS_IO_LIB_H_

#include "../gen_code/cpp/modbus_io_idl_types.h"
#include "../gen_code/cpp/modbus_io_service.h"

namespace modbus_io
{
    void set_one_io(const std::string &_name, bool _is_set);
    bool get_one_io(const std::string &_name);
    void call_remote_modbus_service(std::function<void(modbus_io_serviceClient &)> _func);
};

#endif // _MODBUS_IO_LIB_H_
