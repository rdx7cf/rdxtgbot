#ifndef SQLFILE_H
#define SQLFILE_H

#include <string>
#include <map>
#include <thread>

#include <sqlite3.h>
#include <boost/filesystem.hpp>

class SQLFile
{
public:
    class SQLFileException : public std::runtime_error
    {
    public:
        SQLFileException(const std::string& what) : std::runtime_error(what) {}
    };

    SQLFile(const std::string&, int = 5, int = -1);
    ~SQLFile();

    void sendQuery(const std::string&, int (*)(void*, int, char**, char**) = nullptr, void* = nullptr) const noexcept;
    void autoBackup(std::stop_token) const noexcept;

private:
    mutable std::mutex mtx_sql_;
    std::string filename_;
    sqlite3* connection_;
    mutable char* err_msg_;
    int copies_max_;
    mutable int copies_counter_;
    int interval_;

    void backup() const;
};

#endif
