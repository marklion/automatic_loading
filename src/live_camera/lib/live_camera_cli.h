#if !defined(_LIVE_CAMERA_CLI_H_)
#define _LIVE_CAMERA_CLI_H_

#include "../../public/lib/common_cli.h"

class live_camera_cli : public common_cli
{
public:
    live_camera_cli();
    std::string make_bdr() override;
    virtual void clear() override;
};


#endif // _LIVE_CAMERA_CLI_H_
