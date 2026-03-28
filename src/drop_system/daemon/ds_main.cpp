#include <iostream>
#include "../gen_code/cpp/drop_system_idl_types.h"
#include "../gen_code/cpp/drop_system_service.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"
#include "../../config/lib/config_lib.h"
#include "../../public/lib/modbus_driver.h"
#include "../../public/lib/al_utils.h"
#include "../../modbus_io/lib/modbus_io_lib.h"
#include "../../pid_control/lib/pid_control_lib.h"
#include "../../public/lib/al_utils.h"
#include <fstream>

struct ds_param_runtime
{
    ds_param_info m_param_info;
    std::unique_ptr<modbus_driver> m_driver;
};
struct ds_logger : public modbus_logger
{
    al_log::log_tool m_logger;
    ds_logger(al_log::log_tool &_logger) : m_logger(_logger)
    {
    }
    virtual void log(const char *_fmt, ...)
    {
        char log_buffer[4096] = "";
        va_list args;
        va_start(args, _fmt);
        vsnprintf(log_buffer, sizeof(log_buffer), _fmt, args);
        va_end(args);
        m_logger.log_print(al_log::LOG_LEVEL_ERROR, "%s", log_buffer);
    }
};

std::vector<std::shared_ptr<ds_param_runtime>> g_devices;
static std::unique_ptr<AD_CO_MUTEX> g_mutex = AD_RPC_SC::get_instance()->create_co_mutex();
struct ds_io_runtime
{
    std::string m_input_device_name;
    std::string m_output_on_device_name;
    std::string m_output_off_device_name;
    pid_control::DiscretePID m_pid;
    double m_expect_rate = 0;
    ds_io_runtime(const std::string &_in, const std::string &_on_out, const std::string &_off_out) : m_input_device_name(_in), m_output_on_device_name(_on_out), m_output_off_device_name(_off_out), m_pid(1, 0, 0, 0.05, 10, 0.08)
    {
    }
    ds_io_runtime() : m_pid(1, 0, 0, 0.05, 10, 0.08)
    {
    }
    bool m_is_moving = false;
};
std::map<std::string, ds_io_runtime> g_output_match_map;

class ds_service_imp : public drop_system_serviceIf
{
    al_log::log_tool m_logger;
    AD_EVENT_SC_TIMER_NODE_PTR m_one_time_timer;
    std::shared_ptr<ds_param_runtime> find_param_by_name(const std::string &device_name)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        for (const auto &param_ptr : g_devices)
        {
            if (param_ptr->m_param_info.device_name == device_name)
            {
                return param_ptr;
            }
        }
        return nullptr;
    }

public:
    ds_service_imp() : m_logger(al_log::log_tool(al_log::LOG_DROP_SYSTEM))
    {
    }
    void refresh_driver(bool _force = false)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        for (auto &one_dev : g_devices)
        {
            bool should_refresh = false;
            if (!one_dev->m_driver)
            {
                should_refresh = true;
            }
            else if (one_dev->m_driver && one_dev->m_driver->params_changed(one_dev->m_param_info.ip, one_dev->m_param_info.port, one_dev->m_param_info.slave_id))
            {
                should_refresh = true;
            }
            if (should_refresh || _force)
            {
                one_dev->m_driver.reset();
                one_dev->m_driver = std::make_unique<modbus_driver>(one_dev->m_param_info.ip, one_dev->m_param_info.port, one_dev->m_param_info.slave_id, new ds_logger(m_logger));
                one_dev->m_driver->add_u16_meta("distance", 1);
            }
        }
    }
    virtual bool add_param(const ds_param_info &param_info)
    {
        if (param_info.device_name.length() > 0)
        {
            auto exist_device = find_param_by_name(param_info.device_name);
            if (exist_device == nullptr)
            {
                auto new_one = std::make_shared<ds_param_runtime>();
                new_one->m_param_info = param_info;
                AD_CO_LOCK_GUARD lock(*g_mutex);
                g_devices.push_back(new_one);
            }
            else
            {
                exist_device->m_param_info.ip = param_info.ip;
                exist_device->m_param_info.port = param_info.port;
                exist_device->m_param_info.min_value = param_info.min_value;
                exist_device->m_param_info.max_value = param_info.max_value;
            }
            refresh_driver();
        }

        return true;
    }
    virtual void del_param(const std::string &device_name)
    {
        auto exist_device = find_param_by_name(device_name);
        if (exist_device)
        {
            AD_CO_LOCK_GUARD lock(*g_mutex);
            g_devices.erase(
                std::remove_if(
                    g_devices.begin(), g_devices.end(),
                    [&](const std::shared_ptr<ds_param_runtime> &ptr)
                    { return ptr->m_param_info.device_name == device_name; }),
                g_devices.end());
            refresh_driver();
        }
    }
    virtual void get_all_params(std::vector<ds_param_info> &_return)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        for (const auto &param_ptr : g_devices)
        {
            _return.push_back(param_ptr->m_param_info);
        }
    }
    virtual void readout(ds_readout &_return, const std::string &device_name)
    {
        auto exist_device = find_param_by_name(device_name);
        if (exist_device)
        {
            auto range = exist_device->m_param_info.max_value - exist_device->m_param_info.min_value;
            if (exist_device && exist_device->m_driver)
            {
                auto value = exist_device->m_driver->read_u16("distance");
                auto rate = (value - exist_device->m_param_info.min_value) / range;
                if (rate > 1)
                {
                    rate = 1;
                }
                else if (rate < 0)
                {
                    rate = 0;
                }
                _return.value = value;
                _return.rate = rate;
            }
            if (_return.value > (exist_device->m_param_info.max_value + range * 0.1) ||
                _return.value < (exist_device->m_param_info.min_value - range * 0.1))
            {
                m_logger.log_print(al_log::LOG_LEVEL_ERROR, "readout value %f of device %s is out of range", _return.value, device_name.c_str());
                _return.rate = 1;
            }
        }
    }

    virtual bool add_output_match(const std::string &input_device_name, const ds_input_output &output_match)
    {
        if (input_device_name.length() > 0)
        {
            AD_CO_LOCK_GUARD lock(*g_mutex);
            g_output_match_map[input_device_name] = ds_io_runtime(input_device_name, output_match.output_on_device_name, output_match.output_off_device_name);
        }

        return true;
    }
    virtual void del_output_match(const std::string &input_device_name)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        auto iter = g_output_match_map.find(input_device_name);
        if (iter != g_output_match_map.end())
        {
            g_output_match_map.erase(iter);
        }
    }

    virtual void get_all_output_match(std::vector<ds_input_output> &_return)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        for (auto &itr : g_output_match_map)
        {
            ds_input_output tmp;
            tmp.input_device_name = itr.first;
            tmp.output_on_device_name = itr.second.m_output_on_device_name;
            tmp.output_off_device_name = itr.second.m_output_off_device_name;
            _return.push_back(tmp);
        }
    }

    virtual void set_output(const double expect_rate, const std::string &input_device_name)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        auto iter = g_output_match_map.find(input_device_name);
        if (iter != g_output_match_map.end())
        {
            iter->second.m_expect_rate = expect_rate;
        }
    }
    virtual void turn_on_off(const bool on)
    {
        auto &ci = config::root_config::get_instance();
        ci[CONFIG_ITEM_DS_PID_ON] = on ? "1" : "0";
        AD_CO_LOCK_GUARD lock(*g_mutex);
        for (auto &itr : g_output_match_map)
        {
            itr.second.m_pid.reset();
        }
    }
    virtual bool is_turned_on()
    {
        bool ret = false;
        auto &ci = config::root_config::get_instance();
        auto on_str = ci[CONFIG_ITEM_DS_PID_ON]();
        if (on_str == "1")
        {
            ret = true;
        }
        return ret;
    }

    virtual bool is_moved_by_pid(const std::string &input_device_name)
    {
        AD_CO_LOCK_GUARD lock(*g_mutex);
        auto iter = g_output_match_map.find(input_device_name);
        if (iter != g_output_match_map.end())
        {
            return iter->second.m_is_moving;
        }
        return false;
    }
    virtual void open_one_time(const std::string &input_device_name)
    {
        if (!m_one_time_timer)
        {
            set_output(0.8, input_device_name);
            auto start_time = time(nullptr);
            m_one_time_timer = AD_RPC_SC::get_instance()->startTimer(
                0,
                300,
                [this, input_device_name, start_time]()
                {
                    ds_readout tmp;
                    readout(tmp, input_device_name);
                    auto now_time = time(nullptr);
                    if (tmp.rate >= 0.7 || now_time - start_time > 6)
                    {
                        set_output(0, input_device_name);
                        if (m_one_time_timer)
                        {
                            AD_RPC_SC::get_instance()->stopTimer(m_one_time_timer);
                            m_one_time_timer.reset();
                        }
                    }
                });
        }
    }
};

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_DROP_SYSTEM_SERVER_PORT);
    auto dssi = std::make_shared<ds_service_imp>();
    sc->add_rpc_server(std::make_shared<drop_system_serviceProcessor>(dssi));
    sc->startTimer(
        0,
        80,
        [&]
        {
            bool need_refresh = false;
            {
                AD_CO_LOCK_GUARD lock(*g_mutex);
                for (auto &itr : g_devices)
                {
                    auto modbus_exception = itr->m_driver ? itr->m_driver->exception_info() : "";
                    if (modbus_exception.length() > 0)
                    {
                        need_refresh = true;
                        al_utils::record_self_health(itr->m_param_info.device_name + " modbus driver exception: " + modbus_exception);
                    }
                    else
                    {
                        al_utils::record_self_health("");
                    }
                }
            }
            if (need_refresh)
            {
                dssi->refresh_driver(true);
            }
            if (dssi->is_turned_on())
            {
                std::string one_record = al_utils::ad_utils_date_time().m_datetime_ms;
                AD_CO_LOCK_GUARD lock(*g_mutex);
                for (auto &dev : g_output_match_map)
                {
                    auto input_dev_name = dev.second.m_input_device_name;
                    auto output_on_dev_name = dev.second.m_output_on_device_name;
                    auto output_off_dev_name = dev.second.m_output_off_device_name;
                    auto expect_rate = dev.second.m_expect_rate;
                    ds_readout readout;
                    dssi->readout(readout, input_dev_name);
                    auto measure_value = readout.rate;
                    auto pid_output = dev.second.m_pid.execute(measure_value, expect_rate);
                    if (pid_output > 0)
                    {
                        dev.second.m_is_moving = true;
                        modbus_io::set_one_io(output_off_dev_name, false, "ds_pid_>0");
                        modbus_io::set_one_io(output_on_dev_name, true, "ds_pid_>0");
                    }
                    else if (pid_output == 0)
                    {
                        dev.second.m_is_moving = false;
                        modbus_io::set_one_io(output_on_dev_name, false, "ds_pid=0");
                        modbus_io::set_one_io(output_off_dev_name, false, "ds_pid=0");
                    }
                    else
                    {
                        dev.second.m_is_moving = true;
                        modbus_io::set_one_io(output_on_dev_name, false, "ds_pid_<0");
                        modbus_io::set_one_io(output_off_dev_name, true, "ds_pid_<0");
                    }
                    one_record += "," + input_dev_name + "," + std::to_string(measure_value) + "," + std::to_string(expect_rate) + "," + std::to_string(pid_output);
                }
                std::ofstream ofs("/database/ds_pid_info.csv", std::ios::app);
                ofs << one_record << std::endl;
            }
        });
    al_utils::start_server_notify_started("drop_system");
    return 0;
}
