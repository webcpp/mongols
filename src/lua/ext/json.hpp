#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <memory>
#include "lib/json/json.h"

namespace mongols {

    class json {
    private:
        json(const Json::Value&);
        void config_builder();
    public:
        json();
        json(const json&);
        virtual~json();

        void set_string(const std::string&, const std::string&);
        void set_json(const std::string&, const json&);
        void set_double(const std::string&, double);
        void set_long(const std::string&, long);


        std::string to_string();

        double as_double()const;
        long as_long()const;
        std::string as_string()const;


        bool parse_string(const std::string&);
        bool parse_file(const std::string&);

        json get_object(const std::string&);
        json get_element(int);

        void append_string(const std::string&);
        void append_double(double);
        void append_long(long);


    private:
        Json::Value root;
        Json::StreamWriterBuilder builder;
        Json::Reader reader;
    };
}

#endif /* JSON_HPP */

