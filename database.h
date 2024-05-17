#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <functional>


#include <tgbot/tgbot.h>
#include <boost/filesystem.hpp>
#include <sqlite3.h>

#include "logger.h"


class Database
{
public:
    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const std::string&, std::function<void(const std::string&, const std::string&)>);


    bool contains(const TgBot::User::Ptr&);

    void user_add(const TgBot::User::Ptr&);
    void user_update(const TgBot::User::Ptr&);

private:

    std::vector<TgBot::User::Ptr> users_vec_;
    std::string filename_;
    std::string last_err_msg_;

    std::mutex mutex_sql_;
    std::mutex mutex_vec_;

    void copy_sql_file() const;
};
