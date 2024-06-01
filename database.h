#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <functional>
#include <algorithm>


#include <tgbot/tgbot.h>
#include <boost/filesystem.hpp>
#include <sqlite3.h>

#include "logger.h"
#include "userextended.h"
#include "ad.h"

template<class T>
class Database
{
public:
    typedef std::unique_ptr<Database> uPtr;
    typedef std::shared_ptr<T> dataPtr;

    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const std::string&) = delete;
    void add(const dataPtr&) = delete;
    void update(const dataPtr&) = delete;
    void sync() = delete;
    void show_table(std::ostream&) = delete;


    bool contains(const dataPtr&);
    bool contains(const std::int64_t&);

private:
    friend class BotExtended;

    std::vector<dataPtr> vec_;
    std::string filename_;
    std::string last_err_msg_;

    std::mutex mutex_sql_;
    std::mutex mutex_vec_;

    void copy_sql_file();
    void send_query(const std::string&, int (*)(void*, int, char**, char**) = nullptr, void* = nullptr);
};

template<> Database<UserExtended>::Database(const std::string&);
template<> Database<Ad>::Database(const std::string&);

template<> void Database<UserExtended>::add(const dataPtr&);
template<> void Database<UserExtended>::update(const dataPtr&);
template<> void Database<UserExtended>::sync();
template<> void Database<UserExtended>::show_table(std::ostream&);

template<> void Database<Ad>::add(const dataPtr&);
template<> void Database<Ad>::update(const dataPtr&);
template<> void Database<Ad>::sync();
template<> void Database<Ad>::show_table(std::ostream&);


#include "database.hpp"
