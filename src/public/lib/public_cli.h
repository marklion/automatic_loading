#if !defined(_PUBLIC_CLI_H_)
#define _PUBLIC_CLI_H_

#include "common_cli.h"

class public_cli : public common_cli
{
public:
    public_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _PUBLIC_CLI_H_
