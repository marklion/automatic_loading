#include "config_lib.h"
#include "../../public/lib/al_utils.h"
#include <iostream>
#include <fstream>
#include "../../public/lib/CJsonObject.hpp"

namespace config
{
    config_item config_item::m_empty_item("");
    root_config *root_config::m_instance = nullptr;
    neb::CJsonObject config_item::to_json_object()
    {
        neb::CJsonObject json_obj;
        if (m_value.empty())
        {
            for (const auto &pair : m_children)
            {
                if (pair.second->m_value.empty())
                {
                    json_obj.Add(pair.first, pair.second->to_json_object());
                }
                else
                {
                    json_obj.Add(pair.first, pair.second->m_value);
                }
            }
        }
        else
        {
            json_obj.Add(m_key, m_value);
        }
        return json_obj;
    }
    config_item &config_item::get_child(const std::string &_index_key) const
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
        m_children[_index_key]->m_parent = this;
        save_to_file();
    }
    void config_item::set_child(const std::string &_index_key, const std::string &_value)
    {
        m_children[_index_key] = std::make_shared<config_item>(_index_key, _value);
        m_children[_index_key]->m_parent = this;
        save_to_file();
    }
    void config_item::set_child(const std::string &_index_key)
    {
        if (m_children.find(_index_key) != m_children.end())
        {
            return;
        }
        m_children[_index_key] = std::make_shared<config_item>(_index_key);
        m_children[_index_key]->m_parent = this;
        save_to_file();
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
    void config_item::save_to_file()
    {
        if (!m_parent && m_key == "root")
        {
            std::ifstream self_name("/proc/self/comm");
            std::string exe_path;
            std::getline(self_name, exe_path);
            std::string json_file = "/tmp/" + exe_path + "_config.json";
            neb::CJsonObject json_obj = this->to_json_object();
            std::ofstream ofs(json_file, std::ios::trunc);
            ofs << json_obj.ToFormattedString();
            ofs.close();
        }
        else if (m_parent)
        {
            m_parent->save_to_file();
        }
    }
    root_config &root_config::get_instance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new root_config();
            if (false)
            {
                std::ifstream self_name("/proc/self/comm");
                std::string exe_path;
                std::getline(self_name, exe_path);
                std::string json_file = "/tmp/" + exe_path + "_config.json";
                std::ifstream ifs(json_file);
                if (ifs.is_open())
                {
                    std::string json_content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
                    neb::CJsonObject json_obj(json_content);
                    std::function<void(neb::CJsonObject &, config_item &)> parse_json;
                    parse_json = [&parse_json](neb::CJsonObject &jobj, config_item &citem)
                    {
                        std::vector<std::string> keys;
                        std::string tmp_key;
                        while (jobj.GetKey(tmp_key))
                        {
                            keys.push_back(tmp_key);
                        }
                        for (const auto &key : keys)
                        {
                            std::string value;
                            if (!jobj.Get(key, value))
                            {
                                config_item child_item(key);
                                parse_json(jobj[key], child_item);
                                citem.set_child(key, child_item);
                            }
                            else
                            {
                                citem.set_child(key, value);
                            }
                        }
                    };
                    parse_json(json_obj, *m_instance);
                }
            }
        }
        return *m_instance;
    }
};
