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
#include "ctime++.h"

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

    virtual void sync() = 0;
    virtual void show_table(std::ostream&) = 0;


protected:
    std::string filename_;
    std::string last_err_msg_;

    static std::mutex mutex_sql_;
    std::mutex mutex_vec_;

    void copy_sql_file();
    void send_query(const std::string&, int (*)(void*, int, char**, char**) = nullptr, void* = nullptr);
};

class Userbase : public Database
{
public:
    typedef std::shared_ptr<Userbase> Ptr;
    typedef std::vector<UserExtended::Ptr>::iterator iterator;

    Userbase(const std::string&);

    bool add(const UserExtended::Ptr&);
    bool update(const TgBot::User::Ptr&);

    void sync() override;
    void show_table(std::ostream&) override;

    void for_range(const std::function<void(UserExtended::Ptr&)>&);
    UserExtended::Ptr get_copy_by_id(const std::int64_t&);

private:
    iterator get_by_id(const std::int64_t&);
    std::vector<UserExtended::Ptr> vec_;
};

class Adbase : public Database
{
public:
    typedef std::shared_ptr<Adbase> Ptr;
    typedef std::vector<Ad::Ptr>::iterator iterator;

    Adbase(const std::string&);
    bool add(const Ad::Ptr&);
    bool update(const Ad::Ptr&);

    void sync() override;
    void show_table(std::ostream&) override;

    void for_range(const std::function<void(Ad::Ptr&)>&);
    Ad::Ptr get_copy_by_id(const std::int64_t&);

private:
    iterator get_by_id(const std::int64_t&);
    std::vector<Ad::Ptr> vec_;
};

std::vector<TmExtended> extract_schedule(const std::string&);
