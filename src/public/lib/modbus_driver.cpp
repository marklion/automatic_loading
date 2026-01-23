#include "modbus_driver.h"
float convertRegistersToFloat(uint16_t reg0, uint16_t reg1)
{
    // 创建字节数组
    uint8_t bytes[4];

    // ABCD顺序：reg0是AB，reg1是CD
    // 大端序：高地址存高位
    bytes[0] = (reg0 >> 8) & 0xFF; // A
    bytes[1] = reg0 & 0xFF;        // B
    bytes[2] = (reg1 >> 8) & 0xFF; // C
    bytes[3] = reg1 & 0xFF;        // D

    // 转换为浮点数
    uint32_t int_val = 0;
    int_val = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

    // 转换为浮点数
    float result;
    memcpy(&result, &int_val, sizeof(float));

    return result;
}

modbus_driver::modbus_driver(const std::string &_ip, unsigned short _port, int _slave_id, modbus_logger *_logger):m_logger(_logger)
{
    auto ret = modbus_new_tcp(_ip.c_str(), _port);
    if (ret)
    {
        modbus_set_response_timeout(ret, 0, 500000);
        modbus_set_byte_timeout(ret, 0, 500000);
        if (modbus_connect(ret) == -1)
        {
            m_logger->log("modbus_connect failed: %s", modbus_strerror(errno));
            modbus_free(ret);
            ret = nullptr;
        }
        modbus_set_slave(ret, _slave_id);
    }
    else
    {
        m_logger->log("modbus_new_tcp failed:%s", modbus_strerror(errno));
    }
    m_ctx = ret;
    if (m_ctx)
    {
        m_is_working = true;
        m_work_thread = new std::thread(
            [this]()
            {
                m_logger->log("modbus_driver thread started");
                while (m_is_working)
                {
                    m_mutex.lock();
                    auto tmp_float32_meta = m_float32_abcd_meta;
                    auto tmp_coil_write_meta = m_coil_write_meta;
                    auto tmp_coil_read_meta = m_coil_read_meta;
                    m_mutex.unlock();
                    for (auto &itr : tmp_float32_meta)
                    {
                        auto addr = itr.second.addr;
                        unsigned short reg_buf[2] = {0};
                        auto modbus_ret = modbus_read_registers(m_ctx, addr, 2, reg_buf);
                        if (modbus_ret == 2)
                        {
                            itr.second.value = convertRegistersToFloat(reg_buf[0], reg_buf[1]);
                        }
                        else
                        {
                            m_logger->log("modbus_read_registers failed: %s", modbus_strerror(errno));
                            exception_occurred = true;
                        }
                    }
                    for (auto &itr : tmp_coil_write_meta)
                    {
                        auto addr = itr.second.addr;
                        auto modbus_ret = modbus_write_bit(m_ctx, addr, itr.second.value);
                        if (1 != modbus_ret)
                        {
                            m_logger->log("modbus_write_bit failed: %s", modbus_strerror(errno));
                            exception_occurred = true;
                        }
                    }
                    for (auto &itr : tmp_coil_read_meta)
                    {
                        auto addr = itr.second.addr;
                        unsigned char coil_value = 0;
                        auto modbus_ret = modbus_read_input_bits(m_ctx, addr, 1, &coil_value);
                        if (modbus_ret == 1)
                        {
                            itr.second.value = (coil_value != 0);
                        }
                        else
                        {
                            m_logger->log("modbus_read_bits failed: %s", modbus_strerror(errno));
                            exception_occurred = true;
                        }
                    }
                    m_mutex.lock();
                    m_float32_abcd_meta = tmp_float32_meta;
                    m_coil_read_meta = tmp_coil_read_meta;
                    m_mutex.unlock();
                    usleep(1000 * 200);
                }
                m_logger->log("modbus_driver thread stopped");
            });
    }
    else
    {
        m_logger->log("modbus context is null");
        exception_occurred = true;
    }
}

modbus_driver::~modbus_driver()
{
    if (m_ctx)
    {
        modbus_close(m_ctx);
        modbus_free(m_ctx);
        m_ctx = nullptr;
    }
    m_is_working = false;
    if (m_work_thread)
    {
        m_work_thread->join();
        delete m_work_thread;
        m_work_thread = nullptr;
    }
}

void modbus_driver::add_float32_abcd_meta(const std::string &_name, int addr)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    float_addr_pair pair;
    pair.addr = addr;
    pair.value = 0.0f;
    m_float32_abcd_meta[_name] = pair;
}

void modbus_driver::add_coil_write_meta(const std::string &_name, int addr)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    coil_addr_pair pair;
    pair.addr = addr;
    pair.value = false;
    m_coil_write_meta[_name] = pair;
}

void modbus_driver::del_coil_write_meta(const std::string &_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coil_write_meta.erase(_name);
}

void modbus_driver::add_coil_read_meta(const std::string &_name, int addr)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    coil_addr_pair pair;
    pair.addr = addr;
    pair.value = false;
    m_coil_read_meta[_name] = pair;
}

void modbus_driver::del_coil_read_meta(const std::string &_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coil_read_meta.erase(_name);
}

float modbus_driver::read_float32_abcd(const std::string &_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    float ret = 0.0f;
    auto itr = m_float32_abcd_meta.find(_name);
    if (itr != m_float32_abcd_meta.end())
    {
        ret = itr->second.value;
    }

    return ret;
}

void modbus_driver::write_coil(const std::string &_name, bool _value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_coil_write_meta.find(_name);
    if (itr != m_coil_write_meta.end())
    {
        itr->second.value = _value;
    }
}

bool modbus_driver::read_coil(const std::string &_name)
{
    bool ret = false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto itr = m_coil_read_meta.find(_name);
    if (itr != m_coil_read_meta.end())
    {
        ret = itr->second.value;
    }

    return ret;
}
