#if !defined(_HN_HHT_CLI_H_)
#define _HN_HHT_CLI_H_


#include "../../public/lib/common_cli.h"

class hn_hht_cli : public common_cli
{
public:
    hn_hht_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _HN_HHT_CLI_H_
