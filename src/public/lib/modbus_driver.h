#if !defined(_MODBUS_DRIVER_H_)
#define _MODBUS_DRIVER_H_
#include <string>
#include <map>
#include <modbus/modbus.h>
#include <thread>
#include <mutex>
#include "ad_rpc.h"
struct float_addr_pair{
    int addr;
    float value;
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
    std::map<std::string, coil_addr_pair> m_coil_write_meta;
    std::map<std::string, coil_addr_pair> m_coil_read_meta;
    modbus_t *m_ctx;
    bool m_is_working = false;
    bool exception_occurred = false;
    std::mutex m_mutex;
    std::unique_ptr<modbus_logger> m_logger;
    std::thread *m_work_thread = nullptr;
public:
    modbus_driver(const std::string &_ip, unsigned short _port, int _slave_id, modbus_logger *_logger);
    ~modbus_driver();
    void add_float32_abcd_meta(const std::string &_name, int addr);
    void add_coil_write_meta(const std::string &_name, int addr);
    void del_coil_write_meta(const std::string &_name);
    void add_coil_read_meta(const std::string &_name, int addr);
    void del_coil_read_meta(const std::string &_name);
    float read_float32_abcd(const std::string &_name);
    void write_coil(const std::string &_name, bool _value);
    bool read_coil(const std::string &_name);
};

#endif // _MODBUS_DRIVER_H_
