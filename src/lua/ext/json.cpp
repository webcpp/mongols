#include <fstream>
#include <sstream>
#include "json.hpp"

namespace mongols {

    json::json() : root(), builder(), reader() {
        this->config_builder();
    }

    json::json(const Json::Value& v) : root(v), builder(), reader() {
        this->config_builder();
    }

    json::json(const json& v) : root(v.root), builder(), reader() {
        this->config_builder();
    }

    json::~json() {

    }

    void json::config_builder() {
        this->builder["commentStyle"] = "None";
        this->builder["indentation"] = ""; // or whatever you like
    }

    bool json::parse_string(const std::string& str) {
        return this->reader.parse(str, this->root);
    }

    bool json::parse_file(const std::string& path) {
        std::ifstream ifs(path);
        if (ifs) {
            return this->reader.parse(ifs, this->root);
        }
        return false;
    }

    json json::get_object(const std::string& name) {
        return json(this->root[name]);
    }

    json json::get_element(int i) {
        return json(this->root[i]);
    }

    void json::set_json(const std::string& name, const json& j) {
        this->root[name] = j.root;
    }

    void json::set_double(const std::string& name, double j) {
        this->root[name] = j;
    }

    void json::set_long(const std::string& name, long j) {
        this->root[name] = j;
    }

    void json::set_string(const std::string& name, const std::string& j) {
        this->root[name] = j;
    }

    void json::append_string(const std::string& j) {
        this->root.append(j);
    }

    void json::append_double(double j) {
        this->root.append(j);
    }

    void json::append_long(long j) {
        this->root.append(j);
    }

    std::string json::as_string()const {
        return this->root.asString();
    }

    long json::as_long() const {
        return this->root.asInt64();
    }

    double json::as_double() const {
        return this->root.asDouble();
    }

    std::string json::to_string() {
        std::unique_ptr<Json::StreamWriter> writer(this->builder.newStreamWriter());
        std::ostringstream out;
        writer->write(this->root, &out);
        return out.str();
    }
}
