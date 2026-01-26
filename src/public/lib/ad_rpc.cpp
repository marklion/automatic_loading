#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/processor/TMultiplexedProcessor.h>
#include "ad_rpc.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

class HALF_BUFFER_HALF_SOCKET : public apache::thrift::transport::TTransport
{
    std::string m_orig_data;
    AD_EVENT_SC_TCP_DATA_NODE_PTR m_data_node;
public:
    HALF_BUFFER_HALF_SOCKET(const std::string &_orig_data, AD_EVENT_SC_TCP_DATA_NODE_PTR _data_node) : m_orig_data(_orig_data), m_data_node(_data_node)
    {
    }
    virtual bool isOpen() const override
    {
        return m_data_node->getFd() >= 0;
    }
    virtual void open() override
    {
    }
    virtual void close() override
    {
    }
    virtual void flush() override
    {
    }
    virtual uint32_t read_virt(uint8_t *buf, uint32_t len) override
    {
        if (m_orig_data.size() > 0)
        {
            uint32_t copy_len = std::min(len, (uint32_t)m_orig_data.size());
            m_orig_data.copy((char *)buf, copy_len);
            m_orig_data = m_orig_data.substr(copy_len);
            return copy_len;
        }
        else
        {
            AD_RPC_SC::get_instance()->unregisterNode(m_data_node);
            AD_RPC_SC::get_instance()->yield_by_fd(m_data_node->getFd());
            int ret = recv(m_data_node->getFd(), buf, len, 0);
            AD_RPC_SC::get_instance()->registerNode(m_data_node);
            if (ret <= 0)
            {
                throw apache::thrift::transport::TTransportException(apache::thrift::transport::TTransportException::END_OF_FILE, "No more data to read.");
            }
            return ret;
        }
    }
};

class my_rpc_trans : public AD_EVENT_SC_TCP_DATA_NODE
{
    std::shared_ptr<apache::thrift::TMultiplexedProcessor> m_processor = std::make_shared<apache::thrift::TMultiplexedProcessor>();

public:
    using AD_EVENT_SC_TCP_DATA_NODE::AD_EVENT_SC_TCP_DATA_NODE;
    void handleRead(const unsigned char *buf, size_t len) override
    {
        auto it = std::make_shared<HALF_BUFFER_HALF_SOCKET>(std::string((char *)buf, len), std::static_pointer_cast<AD_EVENT_SC_TCP_DATA_NODE>(shared_from_this()));
        auto it_ad_trans = std::make_shared<AD_RPC_TRANSPORT>(it);
        auto ip = std::make_shared<apache::thrift::protocol::TBinaryProtocol>(it_ad_trans);
        auto ot = std::make_shared<apache::thrift::transport::TMemoryBuffer>();
        auto ot_ad_trans = std::make_shared<AD_RPC_TRANSPORT>(ot);
        auto op = std::make_shared<apache::thrift::protocol::TBinaryProtocol>(ot_ad_trans);
        m_processor->process(ip, op, nullptr);
        auto reply = ot->getBufferAsString();
        if (!reply.empty())
        {
            send(getFd(), reply.c_str(), reply.size(), SOCK_NONBLOCK);
        }
    }
    void add_processor(const std::string &service_name, std::shared_ptr<apache::thrift::TProcessor> processor)
    {
        m_processor->registerProcessor(service_name, processor);
    }
};

AD_EVENT_SC_TCP_DATA_NODE_PTR AD_RPC_EVENT_NODE::create_rpc_tcp_data_node(int fd, AD_EVENT_SC_TCP_LISTEN_NODE_PTR listen_node)
{
    auto ret = std::make_shared<my_rpc_trans>(fd, listen_node);

    for (auto &pair : m_processor_map)
    {
        ret->add_processor(pair.first, pair.second);
    }

    return ret;
}

std::shared_ptr<AD_RPC_SC> AD_RPC_SC::m_single;
std::map<AD_RPC_SERVER_PORTS, std::string> AD_RPC_SC::AD_RPC_SERVER_PORTS_NAME = {
    {AD_RPC_LOG_SERVER_PORT, "log_service"},
    {AD_RPC_PROCESS_SERVER_PORT, "process_service"},
    {AD_RPC_MODBUS_IO_SERVER_PORT, "modbus_io_service"},
    {AD_RPC_SM_SERVER_PORT, "sm_service"},
    {AD_RPC_LIDAR_SERVER_PORT, "lidar_service"},
    {AD_RPC_XLRD_SERVER_PORT, "xlrd_service"},
    {AD_RPC_LIVE_STREAM_SERVER_PORT, "live_camera_service"},
    {AD_RPC_HHT_SERVER_PORT, "hn_hht_service"},
    {AD_RPC_PLATE_GATE_SERVER_PORT, "plate_gate_service"},
    {AD_RPC_SCALE_SERVER_PORT, "scale_service"},
};

void AD_RPC_SC::start_co_record()
{
    startTimer(
        1, []()
        {
            std::string file_name = "/tmp/co_list";
            file_name += std::to_string(getpid()) + ".txt";
            std::ofstream out(file_name, std::ios::trunc);
            out << get_instance()->co_list(); });
}
