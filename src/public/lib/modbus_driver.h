#if !defined(_MODBUS_DRIVER_H_)
#define _MODBUS_DRIVER_H_
#include <string>
#include <map>
#include <modbus/modbus.h>
#include <thread>
#include <mutex>
#include "ad_rpc.h"
#include <atomic>
struct float_addr_pair{
    int addr;
    float value;
};
struct u16_addr_pair{
    int addr;
    uint16_t value;
};
struct coil_addr_pair{
    int addr;
    bool value;
};
struct modbus_logger{
    virtual void log(const char *_fmt, ...) = 0;
};
class modbus_driver {
    std::map<std::string, float_addr_pair> m_float32_abcd_meta;
    std::map<std::string, u16_addr_pair> m_u16_meta;
    std::map<std::string, coil_addr_pair> m_coil_write_meta;
    std::map<std::string, coil_addr_pair> m_coil_read_meta;
    modbus_t *m_ctx;
    std::atomic<bool> m_is_working;
    std::string m_exception_info;
    std::mutex m_mutex;
    std::unique_ptr<modbus_logger> m_logger;
    std::thread *m_work_thread = nullptr;
    void batch_bits_set(std::map<std::string, coil_addr_pair> _coil_write_meta, bool _is_retry = false);
    void batch_bits_get(std::map<std::string, coil_addr_pair> &_coil_read_meta, bool _is_retry = false);
    void batch_float32_abcd_get(std::map<std::string, float_addr_pair> &_float32_abcd_meta);
    void batch_u16_get(std::map<std::string, u16_addr_pair> &_u16_meta, bool _is_retry = false);
    std::string m_ip;
    unsigned short m_port;
    int m_slave_id = 0;
public:
    modbus_driver(const std::string &_ip, unsigned short _port, int _slave_id, modbus_logger *_logger);
    modbus_driver(const std::string &_dev_name, int _baud_rate, int _slave_id);
    ~modbus_driver();
    void setup_modbus(modbus_t *_ctx, int _slave_id = 0);
    void add_float32_abcd_meta(const std::string &_name, int addr);
    void add_coil_write_meta(const std::string &_name, int addr);
    void del_coil_write_meta(const std::string &_name);
    void add_coil_read_meta(const std::string &_name, int addr);
    void del_coil_read_meta(const std::string &_name);
    void add_u16_meta(const std::string &_name, int addr);
    float read_float32_abcd(const std::string &_name);
    void write_coil(const std::string &_name, bool _value);
    bool read_coil(const std::string &_name);
    bool params_changed(const std::string &_ip, unsigned short _port, int _slave_id);
    unsigned short read_u16(const std::string &_name);
    std::string exception_info()
    {
        return m_exception_info;
    }
};

#endif // _MODBUS_DRIVER_H_
