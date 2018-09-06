#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/mman.h>
#include <fstream>
#include <iostream>
#include <cstring>

#include "version.hpp"
#include "web_server.hpp"
#include "util.hpp"
#include "response.hpp"
#include "lib/mustache.hpp"

namespace mongols {

    web_server::web_server(const std::string& host, int port, int timeout, size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
    : cache_expires(3600), root_path(), mime_type(), server(0), list_directory(false), enable_mmap(false) {
        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);
    }

    web_server::~web_server() {
        if (this->server) {
            delete this->server;
        }
    }

    void web_server::run(const std::function<bool(const mongols::request&)>& req_filter) {
        this->server->set_cache_expires(this->cache_expires);
        if (this->enable_mmap) {
            this->server->run(req_filter, std::bind(&web_server::res_filter_with_mmap, this, std::placeholders::_1
                    , std::placeholders::_2));
        } else {
            this->server->run(req_filter, std::bind(&web_server::res_filter, this, std::placeholders::_1
                    , std::placeholders::_2));
        }
    }

    void web_server::res_filter(const mongols::request& req, mongols::response& res) {
        std::string path = this->root_path + req.uri;
        struct stat st;
        if (stat(path.c_str(), &st) >= 0 && S_ISREG(st.st_mode)) {
            int ffd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (ffd > 0) {
                char ffd_buffer[st.st_size];
http_read:
                if (read(ffd, ffd_buffer, st.st_size) < 0) {
                    if (errno == EAGAIN || errno == EINTR) {
                        goto http_read;
                    }

                    close(ffd);
                    goto http_500;
                } else {
                    res.status = 200;
                    res.headers.find("Content-Type")->second = std::move(this->get_mime_type(path));
                    time_t now = time(0);
                    res.headers.insert(std::move(std::make_pair(std::move("Last-Modified"), mongols::http_time(&now))));
                    res.content.assign(ffd_buffer, st.st_size);
                    close(ffd);
                }
            } else {
http_500:
                res.status = 500;
                res.content = std::move("Internal Server Error");
            }
        } else if (S_ISDIR(st.st_mode)) {
            if (this->list_directory) {
                res.content = std::move(this->create_list_directory_response(path));
                res.status = 200;
            } else {
                res.status = 403;
                res.content = std::move("Forbidden");
            }
        }

    }

    std::string web_server::get_mime_type(const std::string& path) {
        std::string::size_type p;
        if (this->mime_type.empty() || (p = path.find_last_of(".")) == std::string::npos) {
            return "application/octet-stream";
        }
        return this->mime_type[path.substr(p + 1)];
    }

    void web_server::set_root_path(const std::string& path) {
        this->root_path = path;
    }

    void web_server::set_mime_type_file(const std::string& path) {
        std::ifstream input(path);
        if (input) {
            std::string line;
            std::vector<std::string> m;
            while (std::getline(input, line)) {
                if (line.front() != '#' && !line.empty()) {
                    split(line, " ", m);
                    int p = 0;
                    for (auto item : m) {
                        if (p++ > 0) {
                            this->mime_type[item] = m[0];
                        }
                    }
                    m.clear();
                }
            }
        }
    }

    std::string web_server::create_list_directory_response(const std::string& path) {
        std::string list_content = std::move("<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<style>"
                "</style>"
                "</head>"
                "<body>"
                "<div>"
                "<h3>Directory index</h3>"
                "<ul>"
                "{{#list}}"
                "<li>"
                "<a href=\"{{href}}\">{{name}}</a>"
                "</li>"
                "{{/list}}"
                "</ul>"
                "</div>"
                "</body>"
                "</html>");
        kainjow::mustache::mustache render_engine(list_content);
        kainjow::mustache::data list{kainjow::mustache::data::type::list};

        DIR * dir = opendir(path.c_str());
        std::string tmp_path;
        size_t n = this->root_path.size();
        struct dirent * entry;
        bool b = path.back() != '/';
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                tmp_path = b ? path + "/" + entry->d_name : path + entry->d_name;
                auto p = tmp_path.find(this->root_path);
                kainjow::mustache::data item;
                item.set("name", entry->d_name);
                item.set("href", tmp_path.substr(p + n));
                list.push_back(item);
            }
        }
        closedir(dir);
        return render_engine.render({"list", list});
    }

    void web_server::set_list_directory(bool b) {
        this->list_directory = b;
    }

    void web_server::set_enable_mmap(bool b) {
        this->enable_mmap = b;
    }

    void web_server::res_filter_with_mmap(const mongols::request& req, mongols::response& res) {
        std::string path = this->root_path + req.uri;
        struct stat st;
        if (stat(path.c_str(), &st) >= 0 && S_ISREG(st.st_mode)) {
            int ffd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (ffd > 0) {
                char *mmap_ptr = (char*) mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, ffd, 0);
                if (mmap_ptr == MAP_FAILED) {
                    close(ffd);
                    goto http_500;
                } else {
                    close(ffd);
                    res.status = 200;
                    res.headers.find("Content-Type")->second = std::move(this->get_mime_type(path));
                    res.content.assign(mmap_ptr, st.st_size);
                    munmap(mmap_ptr, st.st_size);
                }
            } else {
http_500:
                res.status = 500;
                res.content = std::move("Internal Server Error");
            }
        } else if (S_ISDIR(st.st_mode)) {
            if (this->list_directory) {
                res.content = std::move(this->create_list_directory_response(path));
                res.status = 200;
            } else {
                res.status = 403;
                res.content = std::move("Forbidden");
            }
        }

    }

    void web_server::set_cache_expires(long long expires) {
        this->cache_expires = expires;
    }



}

