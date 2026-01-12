#include "config_lib.h"
#include "../../public/lib/al_utils.h"
#include <iostream>

namespace config
{
    config_item config_item::m_empty_item("");
    root_config* root_config::m_instance = nullptr;
    config_item& config_item::get_child(const std::string &_index_key) const
    {
        auto itr = m_children.find(_index_key);
        if (itr == m_children.end())
        {
            return m_empty_item;
        }
        return *(itr->second);
    }

    void config_item::set_child(const std::string &_index_key, const config_item &_item)
    {
        m_children[_index_key] = std::make_shared<config_item>(_item);
    }
    void config_item::set_child(const std::string &_index_key, const std::string &_value)
    {
        m_children[_index_key] = std::make_shared<config_item>(_index_key, _value);
    }
    void config_item::set_child(const std::string &_index_key)
    {
        if (m_children.find(_index_key) != m_children.end())
        {
            return;
        }
        m_children[_index_key] = std::make_shared<config_item>(_index_key);
    }
    std::string config_item::expend_to_string(const std::string &_prefix) const
    {
        std::string ret;
        if (!m_value.empty())
        {
            ret += m_key + "=" + m_value + "\n";
        }
        else
        {
            for (const auto &pair : m_children)
            {
                ret += pair.second->expend_to_string(m_key + "(" + std::to_string(m_children.size()) + ")" + ".");
            }
        }
        auto lines = al_utils::split_string(ret, '\n');
        for (auto &line : lines)
        {
            if (!line.empty())
            {
                line = _prefix + line;
            }
        }
        ret = al_utils::join_strings(lines, ",\n");
        return ret + "\n";
    }
    std::vector<std::unique_ptr<config_item>> config_item::get_children() const
    {
        std::vector<std::unique_ptr<config_item>> children;
        for (const auto &pair : m_children)
        {
            children.push_back(std::make_unique<config_item>(*(pair.second)));
        }
        return children;
    }
    root_config &root_config::get_instance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new root_config();
        }
        return *m_instance;
    }
};
