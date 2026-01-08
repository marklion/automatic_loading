#if !defined(_LOG_CLI_H_)
#define _LOG_CLI_H_

#include "../../public/lib/common_cli.h"

class log_cli : public common_cli
{
public:
    log_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _LOG_CLI_H_
