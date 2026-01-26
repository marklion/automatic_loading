#if !defined(_SCALE_IMP_H_)
#define _SCALE_IMP_H_
#include "../../public/lib/ad_rpc.h"
#include "../gen_code/cpp/scale_idl_types.h"
#include "../gen_code/cpp/scale_service.h"
#include "../../config/lib/config_lib.h"
#include "../../log/lib/log_lib.h"

class SER_FILE_NODE;
typedef std::shared_ptr<SER_FILE_NODE> SER_FILE_NODE_PTR;

class scale_main_impl : public scale_serviceIf
{
    double m_current_weight = 0.0;
    SER_FILE_NODE_PTR m_sfn;
public:
    virtual bool set_params(const scale_config_params &_params);
    virtual void get_params(scale_config_params &_return);
    virtual double read_weight();
    void set_weight(double _weight);
};
class SER_FILE_NODE:public AD_EVENT_SC_NODE{
    int m_ser_fd = -1;
    std::string m_dev_name;
    std::string m_baud_rate;
    scale_main_impl *m_scale_impl = nullptr;
    std::string m_read_buffer;
public:
    SER_FILE_NODE(const std::string &_dev_name, const std::string &_baud_rate, scale_main_impl *_scale_impl):m_dev_name(_dev_name),m_baud_rate(_baud_rate), m_scale_impl(_scale_impl)
    {

    }
    bool prepare_serial_port();
    virtual ~SER_FILE_NODE();
    virtual int getFd() const
    {
        return m_ser_fd;
    }
    // 处理文件描述符上的事件
    virtual void handleEvent();
    virtual std::string node_name() const
    {
        return "serial_file";
    }

};
#endif // _SCALE_IMP_H_
