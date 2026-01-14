#if !defined(_LIDAR_CLI_H_)
#define _LIDAR_CLI_H_

#include "../../public/lib/common_cli.h"

class lidar_cli : public common_cli
{
public:
    lidar_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};

#endif // _LIDAR_CLI_H_
