#pragma once

#include <string>
#include <map>
#include <thread>

#include <sqlite3.h>
#include <boost/filesystem.hpp>

#include "logger.h"

class SQLFile
{
public:
    class sqlfile_exception : public std::runtime_error
    {
    public:
        sqlfile_exception(const std::string& what) : std::runtime_error(what) {}
    };

    SQLFile(const std::string&, int = 5, int = -1);
    ~SQLFile();

    void send_query(const std::string&, int (*)(void*, int, char**, char**) = nullptr, void* = nullptr) const noexcept;
    void auto_backup(std::stop_token) const noexcept;

private:
    mutable std::mutex mtx_sql_;
    std::string filename_;
    sqlite3* connection_;
    mutable char* err_msg_;
    mutable int copies_counter_;
    int interval_;

    void backup() const;
};
