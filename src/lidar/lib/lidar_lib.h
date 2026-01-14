#if !defined(_LIDAR_LIB_H_)
#define _LIDAR_LIB_H_

#include "../gen_code/cpp/lidar_idl_types.h"
#include "../gen_code/cpp/lidar_service.h"
#include "../../public/lib/ad_rpc.h"

namespace ad_lidar
{
    void call_sm_remote(std::function<void(lidar_serviceClient &)> func);
}

#endif // _LIDAR_LIB_H_
