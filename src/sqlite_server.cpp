#include <iostream>
#include "sqlite_server.hpp"
#include "util.hpp"
#include "json/json.h"

#define SQLITE_SQL_FIELD "sql"
#define SQLITE_TYPE_FIELD "sql_type"

namespace mongols {

    sqlite_server::sqlite_server(const std::string& host, int port, int timeout
            , size_t buffer_size, size_t thread_size, size_t max_body_size, int max_event_size)
    : server(0), db(0) {
        this->server = new http_server(host, port, timeout, buffer_size, thread_size, max_body_size, max_event_size);
    }

    sqlite_server::~sqlite_server() {
        if (this->db) {
            delete this->db;
        }
        if (this->server) {
            delete this->server;
        }
    }

    bool sqlite_server::filter(const mongols::request& req) {
        if (req.method == "POST") {
            return true;
        }
        return false;
    }

    void sqlite_server::run(const std::string& db_name) {

        try {
            this->db = new sqlite3pp::database(db_name.c_str());
            this->db->enable_extended_result_codes(true);
            this->db->enable_foreign_keys(true);
            this->db->enable_triggers(true);
            this->server->run(std::bind(&sqlite_server::filter, this, std::placeholders::_1)
                    , std::bind(&sqlite_server::work, this
                    , std::placeholders::_1
                    , std::placeholders::_2));
        } catch (std::exception&) {
        }
    }

    void sqlite_server::work(const mongols::request& req, mongols::response& res) {
        res.headers.find("Content-Type")->second = "application/json;charset=utf-8";
        Json::Value root;
        res.status = 500;
        root["error"] = Json::Value();
        std::unordered_map<std::string, std::string>::const_iterator sql_iter, type_iter;
        if ((sql_iter = req.form.find(SQLITE_SQL_FIELD)) != req.form.end()
                &&(type_iter = req.form.find(SQLITE_TYPE_FIELD)) != req.form.end()) {
            const char* sql = sql_iter->second.c_str();
            if (type_iter->second == "cmd") {
                int rc = this->db->execute(sql);
                if (rc == SQLITE_OK || rc == SQLITE_DONE) {
                    res.status = 200;
                } else {
                    root["error"] = "failed";
                }
            } else if (type_iter->second == "transaction") {
                try {
                    {
                        sqlite3pp::transaction xct(*this->db);
                        {
                            sqlite3pp::command cmd(*this->db, sql);
                            cmd.execute_all();
                        }
                        xct.commit();
                    }
                    res.status = 200;
                } catch (std::exception& e) {
                    root["error"] = e.what();
                }

            } else if (type_iter->second == "query") {
                try {
                    sqlite3pp::query qry(*this->db, sql);
                    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                        Json::Value row;
                        for (int j = 0; j < qry.column_count(); ++j) {
                            switch ((*i).column_type(j)) {
                                case SQLITE_INTEGER:
                                    row[qry.column_name(j)] = static_cast<Json::Int64> ((*i).get<long long int>(j));
                                    break;
                                case SQLITE_FLOAT:
                                    row[qry.column_name(j)] = (*i).get<double>(j);
                                    break;
                                case SQLITE_BLOB:
                                    row[qry.column_name(j)] = std::move((*i).get<std::string>(j));
                                    break;
                                case SQLITE_NULL:
                                    row[qry.column_name(j)] = Json::Value();
                                    break;
                                case SQLITE_TEXT:
                                    row[qry.column_name(j)] = std::move((*i).get<std::string>(j));
                                    break;
                                default:break;
                            }
                        }
                        root["result"].append(row);
                    }
                    res.status = 200;
                } catch (std::exception& e) {
                    root["error"] = e.what();
                }
            } else {
                root["error"] = "Not support this sql_type";
            }
        } else {
            root["error"] = std::move("Not found form data.");
        }
        Json::FastWriter writer;
        res.content = std::move(writer.write(root));
    }




}

