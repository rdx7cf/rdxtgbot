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
    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const std::string& filename) : filename_(filename) {}

    virtual ~Database() {}

    virtual bool contains(const std::int64_t&) = 0;
    virtual void sync() = 0;
    virtual void show_table(std::ostream&) = 0;

protected:
    friend class BotExtended;

    std::string filename_;
    std::string last_err_msg_;

    static std::mutex mutex_sql_;
    std::mutex mutex_vec_;
    std::mutex mutex_database_;

    void copy_sql_file();
    void send_query(const std::string&, int (*)(void*, int, char**, char**) = nullptr, void* = nullptr);
};

class Userbase : public Database
{
public:
    typedef std::shared_ptr<Userbase> Ptr;

    Userbase(const std::string&);

    void add(const UserExtended::Ptr&); // This method also checks whether the vector contains a user to prevent adding multiple rows of the same user.
    void update(const TgBot::User::Ptr&);
    void process_user(const UserExtended::Ptr&);

    bool contains(const std::int64_t&) override;
    void sync() override;
    void show_table(std::ostream&) override;

private:

    friend class BotExtended;
    std::vector<UserExtended::Ptr> vec_;
};

class Adbase : public Database
{
public:
    typedef std::shared_ptr<Adbase> Ptr;

    Adbase(const std::string&);
    void add(const Ad::Ptr&);
    void update(const Ad::Ptr&);
    bool contains(const Ad::Ptr&);

    bool contains(const std::int64_t&) override;
    void sync() override;
    void show_table(std::ostream&) override;

private:

    friend class BotExtended;
    std::vector<Ad::Ptr> vec_;
};

std::vector<std::tm> extract_schedule(const std::string&);
