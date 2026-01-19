#if !defined(_PLATE_GATE_CLI_H_)
#define _PLATE_GATE_CLI_H_

#include "../../public/lib/common_cli.h"

class plate_gate_cli : public common_cli
{
public:
    plate_gate_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _PLATE_GATE_CLI_H_
