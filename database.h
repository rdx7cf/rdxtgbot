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
#include "database.h"


class Database
{
public:

    typedef std::unique_ptr<Database> uPtr;
    typedef std::vector<UserExtended::Ptr>::const_iterator const_iterator;

    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const std::string&);

    bool contains(const TgBot::User::Ptr&);
    bool contains(const std::int64_t&);

    void user_add(const UserExtended::Ptr&);
    void user_update(const TgBot::User::Ptr&);

    void sync();

    const_iterator begin() { return users_vec_.begin(); }
    const_iterator end() { return users_vec_.end(); }

private:
    friend class BotExtended;


    std::vector<UserExtended::Ptr> users_vec_;
    std::string filename_;
    std::string last_err_msg_;

    std::mutex mutex_sql_;
    std::mutex mutex_vec_;

    void copy_sql_file() const;
};
