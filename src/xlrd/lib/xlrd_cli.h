#if !defined(_XLRD_CLI_H_)
#define _XLRD_CLI_H_

#include "../../public/lib/common_cli.h"

class xlrd_cli : public common_cli
{
public:
    xlrd_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _XLRD_CLI_H_
