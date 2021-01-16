#include "sqlite_server.hpp"
#include "lib/json.hpp"
#include "util.hpp"
#include <iostream>

#define SQLITE_SQL_FIELD "sql"
#define SQLITE_TYPE_FIELD "sql_type"

namespace mongols
{

    sqlite_server::sqlite_server(const std::string &host, int port, int timeout, size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
        : server(0), db(0)
    {
        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);
    }

    sqlite_server::~sqlite_server()
    {
        if (this->db)
        {
            delete this->db;
        }
        if (this->server)
        {
            delete this->server;
        }
    }

    bool sqlite_server::filter(const mongols::request &req)
    {
        if (req.method == "POST")
        {
            return true;
        }
        return false;
    }

    void sqlite_server::run(const std::string &db_name)
    {

        try
        {
            this->db = new sqlite3pp::database(db_name.c_str());
            this->db->enable_extended_result_codes(true);
            this->db->enable_foreign_keys(true);
            this->db->enable_triggers(true);
            this->server->run(std::bind(&sqlite_server::filter, this, std::placeholders::_1), std::bind(&sqlite_server::work, this, std::placeholders::_1, std::placeholders::_2));
        }
        catch (std::exception &)
        {
        }
    }

    void sqlite_server::work(const mongols::request &req, mongols::response &res)
    {
        res.headers.find("Content-Type")->second = "application/json;charset=utf-8";
        nlohmann::json root;
        res.status = 500;
        root["error"] = nlohmann::json();
        std::unordered_map<std::string, std::string>::const_iterator sql_iter, type_iter;
        if ((sql_iter = req.form.find(SQLITE_SQL_FIELD)) != req.form.end() && (type_iter = req.form.find(SQLITE_TYPE_FIELD)) != req.form.end())
        {
            const char *sql = sql_iter->second.c_str();
            if (type_iter->second == "cmd")
            {
                int rc = this->db->execute(sql);
                if (rc == SQLITE_OK || rc == SQLITE_DONE)
                {
                    res.status = 200;
                }
                else
                {
                    root["error"] = "failed";
                }
            }
            else if (type_iter->second == "transaction")
            {
                try
                {
                    {
                        sqlite3pp::transaction xct(*this->db);
                        {
                            sqlite3pp::command cmd(*this->db, sql);
                            cmd.execute_all();
                        }
                        xct.commit();
                    }
                    res.status = 200;
                }
                catch (std::exception &e)
                {
                    root["error"] = e.what();
                }
            }
            else if (type_iter->second == "query")
            {
                try
                {
                    sqlite3pp::query qry(*this->db, sql);
                    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i)
                    {
                        nlohmann::json row;
                        for (int j = 0; j < qry.column_count(); ++j)
                        {
                            switch ((*i).column_type(j))
                            {
                            case SQLITE_INTEGER:
                                row[qry.column_name(j)] = static_cast<long long int>((*i).get<long long int>(j));
                                break;
                            case SQLITE_FLOAT:
                                row[qry.column_name(j)] = (*i).get<double>(j);
                                break;
                            case SQLITE_BLOB:
                                row[qry.column_name(j)] = std::move((*i).get<std::string>(j));
                                break;
                            case SQLITE_NULL:
                                row[qry.column_name(j)] = nlohmann::json();
                                break;
                            case SQLITE_TEXT:
                                row[qry.column_name(j)] = std::move((*i).get<std::string>(j));
                                break;
                            default:
                                break;
                            }
                        }
                        root["result"].emplace_back(row);
                    }
                    res.status = 200;
                }
                catch (std::exception &e)
                {
                    root["error"] = e.what();
                }
            }
            else
            {
                root["error"] = "Not support this sql_type";
            }
        }
        else
        {
            root["error"] = std::move("Not found form data.");
        }
        res.content = root.dump();
    }

    void sqlite_server::set_enable_lru_cache(bool b)
    {
        this->server->set_enable_lru_cache(b);
    }

    void sqlite_server::set_lru_cache_expires(long long expires)
    {
        this->server->set_lru_cache_expires(expires);
    }

    void sqlite_server::set_lru_cache_size(size_t len)
    {
        this->server->set_lru_cache_size(len);
    }

    void sqlite_server::set_uri_rewrite(const std::pair<std::string, std::string> &p)
    {
        this->server->set_uri_rewrite(p);
    }

    bool sqlite_server::set_openssl(const std::string &crt_file, const std::string &key_file, openssl::version_t v, const std::string &ciphers, long flags)
    {
        return this->server->set_openssl(crt_file, key_file, v, ciphers, flags);
    }

    void sqlite_server::set_enable_blacklist(bool b)
    {
        this->server->set_enable_blacklist(b);
    }
    void sqlite_server::set_enable_whitelist(bool b)
    {
        this->server->set_enable_whitelist(b);
    }
    void sqlite_server::set_whitelist(const std::string &ip)
    {
        this->server->set_whitelist(ip);
    }
    void sqlite_server::del_whitelist(const std::string &ip)
    {
        this->server->del_whitelist(ip);
    }

    void sqlite_server::set_whitelist_file(const std::string &path)
    {
        this->server->set_whitelist_file(path);
    }
    void sqlite_server::set_enable_security_check(bool b)
    {
        this->server->set_enable_security_check(b);
    }
    void sqlite_server::set_shutdown(const tcp_server::shutdown_function &f)
    {
        this->server->set_shutdown(f);
    }
} // namespace mongols
