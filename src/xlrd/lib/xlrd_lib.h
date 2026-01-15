#if !defined(_XLRD_LIB_H_)
#define _XLRD_LIB_H_

#include "../gen_code/cpp/xlrd_idl_types.h"
#include "../gen_code/cpp/xlrd_service.h"

namespace ad_xlrd
{
    void call_xlrd_service_remote(std::function<void(xlrd_serviceClient &)> func);
}

#endif // _XLRD_LIB_H_
