#if !defined(_SCALE_LIB_H_)
#define _SCALE_LIB_H_

#include "../../public/lib/ad_rpc.h"
#include "../gen_code/cpp/scale_idl_types.h"
#include "../gen_code/cpp/scale_service.h"

namespace al_scale{
    void call_scale_service(std::function<void(scale_serviceClient &)> _func);
};

#endif // _SCALE_LIB_H_
