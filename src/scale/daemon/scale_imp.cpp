#include "scale_imp.h"
#include <sys/unistd.h>
#include <fcntl.h>
#include <termios.h>
static al_log::log_tool g_logger(al_log::LOG_SCALE);
bool scale_main_impl::set_params(const scale_config_params &_params)
{
    auto &ci = config::root_config::get_instance();
    ci.set_child(CONFIG_ITEM_SCALE_DEV_NAME, _params.dev_name);
    ci.set_child(CONFIG_ITEM_SCALE_BAUD_RATE, _params.baud_rate);
    ci.set_child(CONFIG_ITEM_SCALE_WEIGHT_COEFF, std::to_string(_params.weight_coeff));
    if (m_sfn)
    {
        AD_RPC_SC::get_instance()->unregisterNode(m_sfn);
        m_sfn.reset();
    }

    if (_params.dev_name.size())
    {
        m_sfn = std::make_shared<SER_FILE_NODE>(_params.dev_name, _params.baud_rate, this);
        if (m_sfn->prepare_serial_port())
        {
            AD_RPC_SC::get_instance()->registerNode(m_sfn);
        }
    }

    return true;
}

void scale_main_impl::get_params(scale_config_params &_return)
{
    auto &ci = config::root_config::get_instance();
    _return.dev_name = ci(CONFIG_ITEM_SCALE_DEV_NAME);
    _return.baud_rate = ci(CONFIG_ITEM_SCALE_BAUD_RATE);
    _return.weight_coeff = atof(ci(CONFIG_ITEM_SCALE_WEIGHT_COEFF).c_str());
}

double scale_main_impl::read_weight()
{
    return m_current_weight;
}

void scale_main_impl::set_weight(double _weight)
{
    scale_config_params params;
    get_params(params);
    m_current_weight = _weight * params.weight_coeff;
}

bool SER_FILE_NODE::prepare_serial_port()
{
    int fd = open(m_dev_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0)
    {
        g_logger.log_print(al_log::LOG_LEVEL_ERROR, "无法打开串口设备 %s", m_dev_name.c_str());
        return false;
    }
    // 配置串口参数
    struct termios options;
    tcgetattr(fd, &options);
    speed_t baud;
    if (m_baud_rate == "9600")
        baud = B9600;
    else if (m_baud_rate == "19200")
        baud = B19200;
    else if (m_baud_rate == "38400")
        baud = B38400;
    else if (m_baud_rate == "57600")
        baud = B57600;
    else if (m_baud_rate == "115200")
        baud = B115200;
    else
        baud = B9600; // 默认9600
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);
    options.c_cflag |= (CLOCAL | CREAD);                // 本地连接，接收使能
    options.c_cflag &= ~PARENB;                         // 无奇偶校验
    options.c_cflag &= ~CSTOPB;                         // 1位停止位
    options.c_cflag &= ~CSIZE;                          // 清除数据位掩码
    options.c_cflag |= CS8;                             // 8位数据位
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 原始输入模式
    options.c_oflag &= ~OPOST;                          // 原始输出模式
    tcsetattr(fd, TCSANOW, &options);
    m_ser_fd = fd;
    return true;
}

SER_FILE_NODE::~SER_FILE_NODE()
{
    if (m_ser_fd >= 0)
    {
        close(m_ser_fd);
        m_ser_fd = -1;
    }
}

void SER_FILE_NODE::handleEvent()
{
    if (m_ser_fd < 0)
    {
        return;
    }
    char buffer[256];
    ssize_t bytes_read = read(m_ser_fd, buffer, sizeof(buffer) - 1);
    std::string data_str(buffer, bytes_read);
    m_read_buffer += data_str;

    std::string valid_data;
    if (m_read_buffer[0] != '\x02')
    {
        m_read_buffer.erase(0, 1);
        return;
    }

    if (12 <= m_read_buffer.length())
    {
        if (m_read_buffer[11] == '\x03')
        {
            valid_data = m_read_buffer.substr(0, 12);
            m_read_buffer.erase(0, 12);
        }
        else
        {
            m_read_buffer.clear();
        }
    }
    else
    {
        return;
    }
    double weight = 0;
    if (!valid_data.empty())
    {
        auto sign_flag = valid_data[1];

        for (auto i = 0; i < 6; i++)
        {
            auto dig = valid_data[2 + i];
            weight += (dig - '0') * pow(10, (5 - i));
        }
        for (auto i = 0; i < (valid_data[8] - '0'); i++)
        {
            weight /= 10;
        }

        if (sign_flag == '-')
        {
            weight = 0 - weight;
        }
        if (m_scale_impl)
        {
            m_scale_impl->set_weight(weight);
        }
    }
}
