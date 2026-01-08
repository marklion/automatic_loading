#if !defined(_STATE_MACHINE_CLI_H_)
#define _STATE_MACHINE_CLI_H_

#include "../../public/lib/common_cli.h"

class state_machine_cli:public common_cli{
public:
    state_machine_cli();
    virtual std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _STATE_MACHINE_CLI_H_
