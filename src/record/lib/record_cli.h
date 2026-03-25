#if !defined(_RECORD_CLI_H_)
#define _RECORD_CLI_H_

#include "../../public/lib/common_cli.h"

class record_cli : public common_cli
{
public:
    record_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _RECORD_CLI_H_
