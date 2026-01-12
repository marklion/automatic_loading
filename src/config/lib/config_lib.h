#if !defined(_CONFIG_LIB_H_)
#define _CONFIG_LIB_H_

#include <string>
#include <memory>
#include <vector>
#include <map>
namespace config
{
    class config_item
    {
        std::string m_key;
        std::map<std::string, std::shared_ptr<config_item>> m_children;
        std::string m_value;
        static config_item m_empty_item;
    public:
        config_item(const std::string &k) : m_key(k) {}
        config_item(const std::string &_key, const std::string &_value) : m_key(_key), m_value(_value) {}
        std::string get_value() const { return m_value; }
        std::string get_key() const { return m_key; }
        void set_value(const std::string &v) { m_value = v; }
        config_item &get_child(const std::string &_index_key) const;
        void set_child(const std::string &_index_key, const config_item &_item);
        void set_child(const std::string &_index_key, const std::string &_value);
        void set_child(const std::string &_index_key);
        std::string expend_to_string(const std::string &_prefix = "") const;
        void remove_child(const std::string &_index_key)
        {
            m_children.erase(_index_key);
        }
        std::vector<std::unique_ptr<config_item>> get_children() const;
        void operator=(const std::string &_value) { this->set_value(_value); }
        std::string operator()() const { return this->get_value(); }
        std::string operator()(const std::string &_key) const
        {
            return this->get_child(_key).get_value();
        }
        config_item &operator[](const std::string &_key) const
        {
            return this->get_child(_key);
        }
        bool is_empty() const
        {
            return m_key.empty();
        }
    };
    class root_config: public config_item
    {
        root_config() : config_item("root") {}
        static root_config* m_instance;
    public:
        static root_config &get_instance();
    };
};
#define CONFIG_ITEM_STRINGIFY(x) #x
#define CONFIG_ITEM_GEN(_base, _key) CONFIG_ITEM_STRINGIFY(CONFIG_ITEM_##_base##_##_key)

#define CONFIG_ITEM_LOG_FILE CONFIG_ITEM_GEN(LOG, FILE)
#define CONFIG_ITEM_LOG_LEVEL CONFIG_ITEM_GEN(LOG, LEVEL)
#define CONFIG_ITEM_LOG_MODULE CONFIG_ITEM_GEN(LOG, MODULE)

#define CONFIG_ITEM_MODBUS_IO_HOST_NAME CONFIG_ITEM_GEN(MODBUS_IO, HOST_NAME)
#define CONFIG_ITEM_MODBUS_IO_PORT CONFIG_ITEM_GEN(MODBUS_IO, PORT)
#define CONFIG_ITEM_MODBUS_IO_DEVICE_ID CONFIG_ITEM_GEN(MODBUS_IO, DEVICE_ID)

#define CONFIG_ITEM_SM_CONFIG_KITS CONFIG_ITEM_GEN(STATE_MACHINE, CONFIG_KITS)
#define CONFIG_ITEM_SM_CONFIG_MAX_LOAD CONFIG_ITEM_GEN(STATE_MACHINE, MAX_LOAD)
#define CONFIG_ITEM_SM_CONFIG_MAX_FULL_OFFSET CONFIG_ITEM_GEN(STATE_MACHINE, MAX_FULL_OFFSET)
#define CONFIG_ITEM_SM_CONFIG_FRONT_MIN_X CONFIG_ITEM_GEN(STATE_MACHINE, FRONT_MIN_X)
#define CONFIG_ITEM_SM_CONFIG_FRONT_MAX_X CONFIG_ITEM_GEN(STATE_MACHINE, FRONT_MAX_X)
#define CONFIG_ITEM_SM_CONFIG_TAIL_MIN_X CONFIG_ITEM_GEN(STATE_MACHINE, TAIL_MIN_X)
#define CONFIG_ITEM_SM_CONFIG_TAIL_MAX_X CONFIG_ITEM_GEN(STATE_MACHINE, TAIL_MAX_X)
#define CONFIG_ITEM_SM_CONFIG_KIT_OPEN_IO CONFIG_ITEM_GEN(CONFIG_KIT, OPEN_IO)
#define CONFIG_ITEM_SM_CONFIG_KIT_CLOSE_IO CONFIG_ITEM_GEN(CONFIG_KIT, CLOSE_IO)

#endif // _CONFIG_LIB_H_
