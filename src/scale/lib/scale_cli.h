#if !defined(_SCALE_CLI_H_)
#define _SCALE_CLI_H_

#include "../../public/lib/common_cli.h"

class scale_cli : public common_cli
{
public:
    scale_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _SCALE_CLI_H_
