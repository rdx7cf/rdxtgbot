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


class Database
{
public:
    typedef std::unique_ptr<Database> uPtr;

    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const std::string&);

    bool user_contains(const TgBot::User::Ptr&);
    bool user_contains(const std::int64_t&);

    void user_add(const UserExtended::Ptr&);
    void user_update(const TgBot::User::Ptr&);

    bool ad_contains(const std::string&);
    bool ad_contains(const std::int64_t&);

    void ad_add(const Ad::Ptr&);
    void ad_update(const Ad::Ptr&);

    void sync();
    void show_table(std::ostream&);

private:
    friend class BotExtended;

    std::vector<UserExtended::Ptr> users_vec_;
    std::vector<Ad::Ptr> ads_vec_;
    std::string filename_;
    std::string last_err_msg_;

    std::mutex mutex_sql_;
    std::mutex mutex_users_vec_;
    std::mutex mutex_ads_vec_;

    void copy_sql_file() const;
    void send_query();
};
