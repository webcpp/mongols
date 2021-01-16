#include "json.hpp"
#include <fstream>
#include <sstream>

namespace mongols
{

    json::json()
        : root()
    {
    }

    json::json(const nlohmann::json &v)
        : root(v)
    {
    }

    json::json(const json &v)
        : root(v.root)
    {
    }

    json::~json()
    {
    }

    bool json::parse_string(const std::string &str)
    {
        this->root = nlohmann::json::parse(str);
        return true;
    }

    bool json::parse_file(const std::string &path)
    {
        std::ifstream ifs(path);
        if (ifs)
        {
            ifs >> this->root;
            return true;
        }
        return false;
    }

    json json::get_object(const std::string &name)
    {
        return json(this->root[name]);
    }

    json json::get_element(int i)
    {
        return json(this->root[i]);
    }

    void json::set_json(const std::string &name, const json &j)
    {
        this->root[name] = j.root;
    }

    void json::set_double(const std::string &name, double j)
    {
        this->root[name] = j;
    }

    void json::set_long(const std::string &name, long j)
    {
        this->root[name] = j;
    }

    void json::set_bool(const std::string &name, bool j)
    {
        this->root[name] = j;
    }

    void json::set_string(const std::string &name, const std::string &j)
    {
        this->root[name] = j;
    }

    void json::append_string(const std::string &j)
    {
        this->root.emplace_back(j);
    }

    void json::append_double(double j)
    {
        this->root.emplace_back(j);
    }

    void json::append_long(long j)
    {
        this->root.emplace_back(j);
    }

    void json::append_json(const json &j)
    {
        this->root.emplace_back(j.root);
    }

    void json::append_bool(bool j)
    {
        this->root.emplace_back(j);
    }

    std::string json::as_string() const
    {
        return this->root.get<std::string>();
    }

    long json::as_long() const
    {
        return this->root.get<long>();
    }

    double json::as_double() const
    {
        return this->root.get<double>();
    }

    bool json::as_bool() const
    {
        return this->root.get<bool>();
    }

    std::string json::to_string()
    {
        return this->root.dump();
    }

    bool json::is_array() const
    {
        return this->root.is_array();
    }

    bool json::is_double() const
    {
        return this->root.is_number_float();
    }

    bool json::is_long() const
    {
        return this->root.is_number_integer();
    }

    bool json::is_object() const
    {
        return this->root.is_object();
    }

    bool json::is_string() const
    {
        return this->root.is_string();
    }

    bool json::is_bool() const
    {
        return this->root.is_binary();
    }

    size_t json::size() const
    {
        if (this->is_array() || this->is_object())
        {
            return this->root.size();
        }
        return 0;
    }
} // namespace mongols
