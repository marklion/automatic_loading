#include "log_lib.h"
#include "../../public/lib/ad_rpc.h"
#include <thrift/TProcessor.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/processor/TMultiplexedProcessor.h>
#include <thrift/protocol/TMultiplexedProtocol.h>
#include "../gen_code/cpp/log_idl_types.h"
#include "../gen_code/cpp/log_service.h"
#include "../../public/lib/al_utils.h"
#include "../../config/lib/config_lib.h"

static void log_print_to_server(const std::string &_log_msg, const int32_t module_id, const int32_t level_id)
{
    std::string log_msg = _log_msg;
    std::string process_path;
    char buf[1024] = {0};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1)
    {
        buf[len] = '\0';
        process_path = std::string(buf);
    }
    size_t pos = process_path.find_last_of("/");
    std::string process_name = (pos != std::string::npos) ? process_path.substr(pos + 1) : process_path;
    log_msg.insert(0, process_name + "|");
    log_msg.insert(0, std::to_string(getpid()) + "|");
    log_msg.insert(0, al_utils::ad_utils_date_time().m_datetime_ms + "|");
    AD_RPC_SC::get_instance()->call_remote<log_serviceClient>(
        AD_RPC_LOG_SERVER_PORT,
        [log_msg, module_id, level_id](log_serviceClient &client)
        {
            client.log_print(log_msg, module_id, level_id);
        });
}

namespace al_log
{
    std::map<LOG_MODULE, std::string> LOG_MODULE_NAME = {
        {LOG_CONFIG, "CONFIG"},
        {LOG_MODBUS_IO, "MODBUS_IO"},
        {LOG_STATE_MACHINE, "STATE_MACHINE"},
        {LOG_LIDAR, "LIDAR"},
        {LOG_XLRD, "XLRD"},
        {LOG_TEST, "TEST"},
    };
    void log_tool::log_print(LOG_LEVEL _level, const char *_format, ...)
    {
        const char *level_str = "";
        switch (_level)
        {
        case LOG_LEVEL_DEBUG:
            level_str = "DEBUG";
            break;
        case LOG_LEVEL_INFO:
            level_str = "INFO";
            break;
        case LOG_LEVEL_WARN:
            level_str = "WARN";
            break;
        case LOG_LEVEL_ERROR:
            level_str = "ERROR";
            break;
        default:
            level_str = "UNKNOWN";
            break;
        }
        char log_buffer[4096] = "";
        char final_log[8192] = "";
        va_list args;
        va_start(args, _format);
        vsnprintf(log_buffer, sizeof(log_buffer), _format, args);
        snprintf(final_log, sizeof(final_log), "[%s][%s]: %s\n", LOG_MODULE_NAME[m_module].c_str(), level_str, log_buffer);
        va_end(args);
        log_print_to_server(final_log, m_module, _level);
    }
    void call_log_service(std::function<void(log_serviceClient &)> _func)
    {
        AD_RPC_SC::get_instance()->call_remote<log_serviceClient>(
            AD_RPC_LOG_SERVER_PORT,
            [_func](log_serviceClient &client)
            {
                _func(client);
            });
    }
};