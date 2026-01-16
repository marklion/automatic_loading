#if !defined(_LIVE_CAMERA_LIB_H_)
#define _LIVE_CAMERA_LIB_H_

#include "../gen_code/cpp/live_camera_idl_types.h"
#include "../gen_code/cpp/live_camera_service.h"

namespace live_camera
{
    void call_live_camera_remote(std::function<void(live_camera_serviceClient &)> func);
};

#endif // _LIVE_CAMERA_LIB_H_
