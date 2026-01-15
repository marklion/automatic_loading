#include "../gen_code/cpp/xlrd_idl_types.h"
#include "../gen_code/cpp/xlrd_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../config/lib/config_lib.h"
#include "../../public/lib/modbus_driver.h"
#include "../../log/lib/log_lib.h"
#include "../../state_machine/lib/state_machine_lib.h"

struct my_logger : public modbus_logger
{
    al_log::log_tool *m_logger;
    my_logger(al_log::log_tool *_logger) : m_logger(_logger) {}
    virtual void log(const char *_fmt, ...)
    {
        char log_buffer[4096] = "";
        va_list args;
        va_start(args, _fmt);
        vsnprintf(log_buffer, sizeof(log_buffer), _fmt, args);
        va_end(args);
        m_logger->log_print(al_log::LOG_LEVEL_ERROR, "%s", log_buffer);
    }
};

class xlrd_service_imp : public xlrd_serviceIf
{
    std::unique_ptr<modbus_driver> m_front_driver;
    std::unique_ptr<modbus_driver> m_tail_driver;
    al_log::log_tool m_logger;

public:
    xlrd_service_imp() : m_logger(al_log::LOG_XLRD)
    {
        AD_RPC_SC::get_instance()->startTimer(
            0,
            240,
            [this]()
            {
                auto front_distance = this->read_distance(true);
                auto tail_distance = this->read_distance(false);
                bool is_front_dropped = false;
                state_machine::call_sm_remote(
                    [&](state_machine_serviceClient &_client)
                    {
                        state_machine_status tmp_status;
                        _client.get_state_machine_status(tmp_status);
                        is_front_dropped = tmp_status.is_front_dropped;
                        if (is_front_dropped)
                        {
                            _client.push_stuff_full_offset(front_distance);
                        }
                        else
                        {
                            _client.push_stuff_full_offset(tail_distance);
                        }
                    });
            });
    }
    virtual bool set_config_params(const bool _is_front, const xlrd_config_params &_params)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_XLRD_FRONT_PARAMS);
        ci.set_child(CONFIG_ITEM_XLRD_TAIL_PARAMS);
        auto section = _is_front ? CONFIG_ITEM_XLRD_FRONT_PARAMS : CONFIG_ITEM_XLRD_TAIL_PARAMS;
        ci[section].set_child(CONFIG_ITEM_XLRD_PARAMS_IP, _params.ip);
        ci[section].set_child(CONFIG_ITEM_XLRD_PARAMS_PORT, std::to_string(_params.port));
        ci[section].set_child(CONFIG_ITEM_XLRD_PARAMS_SLAVE_ID, std::to_string(_params.slave_id));
        ci[section].set_child(CONFIG_ITEM_XLRD_PARAMS_DISTANCE_OFFSET, std::to_string(_params.distance_offset));
        ci[section].set_child(CONFIG_ITEM_XLRD_PARAMS_BOTTOM_Z, std::to_string(_params.bottom_z));
        modbus_driver *driver_ptr = nullptr;

        if (_is_front)
        {
            m_front_driver = std::make_unique<modbus_driver>(
                _params.ip,
                static_cast<unsigned short>(_params.port),
                _params.slave_id,
                new my_logger(&m_logger));
            driver_ptr = m_front_driver.get();
        }
        else
        {
            m_tail_driver = std::make_unique<modbus_driver>(
                _params.ip,
                static_cast<unsigned short>(_params.port),
                _params.slave_id,
                new my_logger(&m_logger));
            driver_ptr = m_tail_driver.get();
        }
        if (driver_ptr)
        {
            driver_ptr->add_float32_abcd_meta("distance", 4096);
        }

        return true;
    }
    virtual void get_config_params(xlrd_config_params &_return, const bool _is_front)
    {
        auto &ci = config::root_config::get_instance();
        auto section = _is_front ? CONFIG_ITEM_XLRD_FRONT_PARAMS : CONFIG_ITEM_XLRD_TAIL_PARAMS;
        _return.ip = ci[section][CONFIG_ITEM_XLRD_PARAMS_IP]();
        _return.port = atoi(ci[section][CONFIG_ITEM_XLRD_PARAMS_PORT]().c_str());
        _return.slave_id = atoi(ci[section][CONFIG_ITEM_XLRD_PARAMS_SLAVE_ID]().c_str());
        _return.distance_offset = atof(ci[section][CONFIG_ITEM_XLRD_PARAMS_DISTANCE_OFFSET]().c_str());
        _return.bottom_z = atof(ci[section][CONFIG_ITEM_XLRD_PARAMS_BOTTOM_Z]().c_str());
        m_logger.log_print(al_log::LOG_LEVEL_INFO,"xlrd config is:%s", ci.expend_to_string().c_str());
    }
    virtual double read_distance(const bool _is_front)
    {
        double ret = 0;
        modbus_driver *driver_ptr = nullptr;
        if (_is_front)
        {
            driver_ptr = m_front_driver.get();
        }
        else
        {
            driver_ptr = m_tail_driver.get();
        }
        if (driver_ptr)
        {
            ret = static_cast<double>(driver_ptr->read_float32_abcd("distance"));
        }

        return ret;
    }
};

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_XLRD_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<xlrd_serviceProcessor>(std::make_shared<xlrd_service_imp>()));
    sc->start_server();
    return 0;
}
