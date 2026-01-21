#include "../gen_code/cpp/modbus_io_service.h"
#include "../gen_code/cpp/modbus_io_idl_types.h"
#include "../../public/lib/ad_rpc.h"
#include "../../log/lib/log_lib.h"
#include "../../config/lib/config_lib.h"
#include "../../public/lib/modbus_driver.h"

std::vector<std::shared_ptr<modbus_device>> g_devices;
al_log::log_tool g_logger(al_log::LOG_MODBUS_IO);
struct modbus_io_logger : public modbus_logger
{
    al_log::log_tool m_logger;
    modbus_io_logger(al_log::log_tool &_logger) : m_logger(_logger)
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

class modbus_io_service_imp : public modbus_io_serviceIf
{
    al_log::log_tool m_logger;
    std::shared_ptr<modbus_driver> m_driver;
    std::shared_ptr<modbus_driver> get_driver()
    {
        if (!m_driver)
        {
            auto &ci = config::root_config::get_instance();
            auto host_name = ci(CONFIG_ITEM_MODBUS_IO_HOST_NAME);
            auto port = atoi(ci(CONFIG_ITEM_MODBUS_IO_PORT).c_str());
            auto device_id = atoi(ci(CONFIG_ITEM_MODBUS_IO_DEVICE_ID).c_str());
            if (host_name.length() > 0 && port > 0)
            {
                m_driver = std::make_shared<modbus_driver>(host_name, port, device_id, new modbus_io_logger(m_logger));
            }
        }
        if (!m_driver)
        {
            m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Modbus driver is not configured properly");
        }
        if (m_driver && m_driver->exception_happened())
        {
            m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Modbus exception happened, recreating driver");
            m_driver.reset();
        }
        return m_driver;
    }

public:
    modbus_io_service_imp() : m_logger(g_logger)
    {
    }
    std::shared_ptr<modbus_device> find_device_by_name(const std::string &_name)
    {
        for (auto &device_ptr : g_devices)
        {
            if (device_ptr->device_name == _name)
            {
                return device_ptr;
            }
        }
        return nullptr;
    }
    virtual bool add_device(const std::string &device_name, const int32_t channel_id, const bool is_output)
    {
        bool ret = false;
        if (find_device_by_name(device_name) != nullptr)
        {
            m_logger.log_print(al_log::LOG_LEVEL_WARN, "Device %s already exists", device_name.c_str());
        }
        else
        {
            if (channel_id < 0 || channel_id >= 8)
            {
                m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Channel ID %d is out of range", channel_id);
                ad_modbus_io_gen_exp exp;
                exp.msg = "Channel ID is out of range";
                throw exp;
            }
            modbus_device tmp;
            tmp.device_name = device_name;
            tmp.channel_id = channel_id;
            tmp.is_output = is_output;
            g_devices.push_back(std::make_shared<modbus_device>(tmp));
            auto driver = get_driver();
            if (driver)
            {
                if (is_output)
                {
                    driver->add_coil_write_meta(device_name, channel_id);
                }
                else
                {
                    driver->add_coil_read_meta(device_name, channel_id + 0x20);
                }
            }

            ret = true;
        }

        return ret;
    }
    virtual void del_device(const std::string &device_name)
    {
        auto device_ptr = find_device_by_name(device_name);
        if (device_ptr != nullptr)
        {
            g_devices.erase(
                std::remove_if(
                    g_devices.begin(),
                    g_devices.end(),
                    [&](const std::shared_ptr<modbus_device> &ptr)
                    { return ptr->device_name == device_name; }),
                g_devices.end());
            auto driver = get_driver();
            if (driver)
            {
                if (device_ptr->is_output)
                {
                    driver->del_coil_write_meta(device_name);
                }
                else
                {
                    driver->del_coil_read_meta(device_name);
                }
            }
        }
        else
        {
            m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Device %s not found", device_name.c_str());
        }
    }
    virtual void get_all_devices(std::vector<modbus_device> &_return)
    {
        for (const auto &device_ptr : g_devices)
        {
            if (!device_ptr->is_output)
            {
                device_io_get(device_ptr->device_name);
            }
            _return.push_back(*device_ptr);
        }
    }
    virtual bool device_io_set(const std::string &device_name, const bool value)
    {
        bool ret = false;
        auto device_ptr = find_device_by_name(device_name);
        if (device_ptr != nullptr)
        {
            if (device_ptr->is_output)
            {
                device_ptr->is_opened = value;
                auto driver = get_driver();
                if (driver)
                {
                    driver->write_coil(device_name, value);
                }
                ret = true;
            }
            else
            {
                m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Device %s is not an output device", device_name.c_str());
            }
        }
        else
        {
            m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Device %s not found", device_name.c_str());
        }
        return ret;
    }
    virtual bool device_io_get(const std::string &device_name)
    {
        bool ret = false;
        auto device_ptr = find_device_by_name(device_name);
        if (device_ptr != nullptr)
        {
            if (!device_ptr->is_output)
            {
                auto driver = get_driver();
                if (driver)
                {
                    device_ptr->is_opened = driver->read_coil(device_name);
                }
                ret = device_ptr->is_opened;
            }
            else
            {
                m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Device %s is not an input device", device_name.c_str());
            }
        }
        else
        {
            m_logger.log_print(al_log::LOG_LEVEL_ERROR, "Device %s not found", device_name.c_str());
        }
        return ret;
    }

    virtual bool set_modbus_tcp(const modbus_tcp_config &_config)
    {
        auto &ci = config::root_config::get_instance();
        ci.set_child(CONFIG_ITEM_MODBUS_IO_HOST_NAME, _config.host_name);
        ci.set_child(CONFIG_ITEM_MODBUS_IO_PORT, std::to_string(_config.port));
        ci.set_child(CONFIG_ITEM_MODBUS_IO_DEVICE_ID, std::to_string(_config.device_id));
        m_driver.reset();
        auto new_driver = get_driver();
        if (new_driver)
        {
            for (const auto &device_ptr : g_devices)
            {
                if (device_ptr->is_output)
                {
                    new_driver->add_coil_write_meta(device_ptr->device_name, device_ptr->channel_id);
                }
                else
                {
                    new_driver->add_coil_read_meta(device_ptr->device_name, device_ptr->channel_id + 0x20);
                }
            }
        }
        return true;
    }
    virtual void get_modbus_tcp(modbus_tcp_config &_return)
    {
        auto &ci = config::root_config::get_instance();
        _return.host_name = ci(CONFIG_ITEM_MODBUS_IO_HOST_NAME);
        _return.port = atoi(ci(CONFIG_ITEM_MODBUS_IO_PORT).c_str());
        _return.device_id = atoi(ci(CONFIG_ITEM_MODBUS_IO_DEVICE_ID).c_str());
    }
};

int main(int argc, char const *argv[])
{
    auto sc = AD_RPC_SC::get_instance();
    sc->enable_rpc_server(AD_RPC_MODBUS_IO_SERVER_PORT);
    sc->add_rpc_server(std::make_shared<modbus_io_serviceProcessor>(std::make_shared<modbus_io_service_imp>()));
    sc->start_server();
    return 0;
}
