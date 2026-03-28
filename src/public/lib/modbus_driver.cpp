#include "modbus_driver.h"
#include "../../public/lib/al_utils.h"
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

void modbus_driver::batch_bits_set(std::map<std::string, coil_addr_pair> _coil_write_meta)
{
    if (_coil_write_meta.size() > 0)
    {
        int base_addr = _coil_write_meta.begin()->second.addr;
        int reg_num = _coil_write_meta.size();
        uint8_t *write_buf = (uint8_t *)(calloc(sizeof(uint8_t), reg_num));
        for (auto &itr : _coil_write_meta)
        {
            if (itr.second.addr < base_addr)
            {
                base_addr = itr.second.addr;
            }
        }

        for (auto &itr : _coil_write_meta)
        {
            write_buf[itr.second.addr - base_addr] = itr.second.value ? 1 : 0;
        }
        auto modbus_ret = modbus_write_bits(m_ctx, base_addr, reg_num, write_buf);
        if (modbus_ret != reg_num)
        {
            m_exception_info = std::to_string(modbus_ret) + ":" + modbus_strerror(errno);
        }
        free(write_buf);
    }
}

void modbus_driver::batch_bits_get(std::map<std::string, coil_addr_pair> &_coil_read_meta)
{
    if (_coil_read_meta.size() > 0)
    {
        int base_addr = _coil_read_meta.begin()->second.addr;
        int reg_num = _coil_read_meta.size();
        for (auto &itr : _coil_read_meta)
        {
            if (itr.second.addr < base_addr)
            {
                base_addr = itr.second.addr;
            }
        }
        uint8_t *read_buf = (uint8_t *)(calloc(sizeof(uint8_t), reg_num));
        auto modbus_ret = modbus_read_input_bits(m_ctx, base_addr, reg_num, read_buf);
        if (modbus_ret == reg_num)
        {
            for (auto &itr : _coil_read_meta)
            {
                itr.second.value = (read_buf[itr.second.addr - base_addr] != 0);
            }
        }
        else
        {
            m_exception_info = std::to_string(modbus_ret) + ":" + modbus_strerror(errno);
        }
        free(read_buf);
    }
}

void modbus_driver::batch_float32_abcd_get(std::map<std::string, float_addr_pair> &_float32_abcd_meta)
{
    if (_float32_abcd_meta.size() > 0)
    {
        int base_addr = _float32_abcd_meta.begin()->second.addr;
        int reg_num = _float32_abcd_meta.size() * 2;
        for (auto &itr : _float32_abcd_meta)
        {
            if (itr.second.addr < base_addr)
            {
                base_addr = itr.second.addr;
            }
        }
        unsigned short *read_buf = (unsigned short *)(calloc(sizeof(unsigned short), reg_num));
        auto modbus_ret = modbus_read_registers(m_ctx, base_addr, reg_num, read_buf);
        if (modbus_ret == reg_num)
        {
            for (auto &itr : _float32_abcd_meta)
            {
                int offset = itr.second.addr - base_addr;
                itr.second.value = convertRegistersToFloat(read_buf[offset], read_buf[offset + 1]);
            }
        }
        else
        {
            m_exception_info = std::to_string(modbus_ret) + ":" + modbus_strerror(errno);
        }
        free(read_buf);
    }
}

void modbus_driver::batch_u16_get(std::map<std::string, u16_addr_pair> &_u16_meta)
{
    if (_u16_meta.size() > 0)
    {
        int base_addr = _u16_meta.begin()->second.addr;
        int reg_num = _u16_meta.size();
        for (auto &itr : _u16_meta)
        {
            if (itr.second.addr < base_addr)
            {
                base_addr = itr.second.addr;
            }
        }
        unsigned short *read_buf = (unsigned short *)(calloc(sizeof(unsigned short), reg_num));
        auto modbus_ret = modbus_read_registers(m_ctx, base_addr, reg_num, read_buf);
        if (modbus_ret == reg_num)
        {
            for (auto &itr : _u16_meta)
            {
                int offset = itr.second.addr - base_addr;
                itr.second.value = read_buf[offset];
            }
        }
        else
        {
            m_exception_info = std::to_string(modbus_ret) + ":" + modbus_strerror(errno);
        }
        free(read_buf);
    }
}

modbus_driver::modbus_driver(const std::string &_ip, unsigned short _port, int _slave_id, modbus_logger *_logger) : m_logger(_logger), m_ip(_ip), m_port(_port), m_slave_id(_slave_id),m_is_working(false)
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
                    auto tmp_u16_meta = m_u16_meta;
                    m_mutex.unlock();
                    auto start_us_stamp = al_utils::get_current_us_stamp();
                    batch_bits_set(tmp_coil_write_meta);
                    batch_bits_get(tmp_coil_read_meta);
                    batch_float32_abcd_get(tmp_float32_meta);
                    batch_u16_get(tmp_u16_meta);
                    auto end_us_stamp = al_utils::get_current_us_stamp();
                    m_logger->log("modbus_driver loop time: %lld ms", (end_us_stamp - start_us_stamp) / 1000);
                    m_mutex.lock();
                    m_float32_abcd_meta = tmp_float32_meta;
                    m_coil_read_meta = tmp_coil_read_meta;
                    m_u16_meta = tmp_u16_meta;
                    m_mutex.unlock();
                    usleep(1000 * 70);
                }
                m_logger->log("modbus_driver thread stopped");
            });
    }
    else
    {
        m_logger->log("modbus context is null");
        m_exception_info = std::string("open :") + modbus_strerror(errno);
    }
}

modbus_driver::~modbus_driver()
{
    m_is_working = false;
    if (m_work_thread)
    {
        m_work_thread->join();
        delete m_work_thread;
        m_work_thread = nullptr;
    }
    if (m_ctx)
    {
        modbus_close(m_ctx);
        modbus_free(m_ctx);
        m_ctx = nullptr;
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

void modbus_driver::add_u16_meta(const std::string &_name, int addr)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    u16_addr_pair pair;
    pair.addr = addr;
    pair.value = 0;
    m_u16_meta[_name] = pair;
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

bool modbus_driver::params_changed(const std::string &_ip, unsigned short _port, int _slave_id)
{
    bool ret = true;
    if (_ip == m_ip && _port == m_port && _slave_id == m_slave_id)
    {
        ret = false;
    }

    return ret;
}

unsigned short modbus_driver::read_u16(const std::string &_name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    unsigned short ret = 0;
    auto itr = m_u16_meta.find(_name);
    if (itr != m_u16_meta.end())
    {
        ret = itr->second.value;
    }

    return ret;
}
