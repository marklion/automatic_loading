#if !defined(_MODBUS_IO_CLI_H_)
#define _MODBUS_IO_CLI_H_

#include "../../public/lib/common_cli.h"

class modbus_io_cli:public common_cli{
public:
    modbus_io_cli();
    virtual std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _MODBUS_IO_CLI_H_
