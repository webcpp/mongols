#ifndef JSON_HPP
#define JSON_HPP

#include "lib/json.hpp"
#include <memory>
#include <string>

namespace mongols
{

    class json
    {
    private:
        json(const nlohmann::json &);

    public:
        json();
        json(const json &);
        virtual ~json();

        void set_string(const std::string &, const std::string &);
        void set_json(const std::string &, const json &);
        void set_double(const std::string &, double);
        void set_long(const std::string &, long);
        void set_bool(const std::string &, bool);

        std::string to_string();

        double as_double() const;
        long as_long() const;
        std::string as_string() const;
        bool as_bool() const;

        bool parse_string(const std::string &);
        bool parse_file(const std::string &);

        json get_object(const std::string &);
        json get_element(int);

        void append_string(const std::string &);
        void append_double(double);
        void append_long(long);
        void append_json(const json &);
        void append_bool(bool);

        bool is_double() const;
        bool is_long() const;
        bool is_string() const;
        bool is_object() const;
        bool is_array() const;
        bool is_bool() const;

        size_t size() const;

    private:
        nlohmann::json root;
    };
} // namespace mongols

#endif /* JSON_HPP */
