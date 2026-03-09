#if !defined(_DS_CLI_H_)
#define _DS_CLI_H_

#include "../../public/lib/common_cli.h"

class ds_cli:public common_cli{
public:
    ds_cli();
    virtual std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _DS_CLI_H_
