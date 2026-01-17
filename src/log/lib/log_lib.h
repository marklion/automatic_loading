#if !defined(_LOG_LIB_H_)
#define _LOG_LIB_H_
#include <map>
#include <stdarg.h>
#include "../gen_code/cpp/log_service.h"
#include "../gen_code/cpp/log_idl_types.h"

namespace al_log
{
    enum LOG_MODULE
    {
        LOG_CONFIG = 0,
        LOG_MODBUS_IO,
        LOG_STATE_MACHINE,
        LOG_LIDAR,
        LOG_XLRD,
        LOG_LIVE_CAMERA,
        LOG_HHT,
        LOG_TEST,
    };

    enum LOG_LEVEL
    {
        LOG_LEVEL_DEBUG = 0,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_NONE,
    };
    extern std::map<LOG_MODULE, std::string> LOG_MODULE_NAME;
    class log_tool
    {
        LOG_MODULE m_module;

    public:
        log_tool(LOG_MODULE _module) : m_module(_module) {}
        void log_print(LOG_LEVEL _level, const char *_format, ...);
    };
    void call_log_service(std::function<void(log_serviceClient &)> _func);
};

#endif // _LOG_LIB_H_
