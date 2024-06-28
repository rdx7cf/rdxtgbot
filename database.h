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
#include "notification.h"
#include "ctime++.h"

inline std::mutex mutex_sql_ {};

template<typename T>
class Database
{
public:
    using PtrT = std::shared_ptr<T>;
    using iterator = std::vector<PtrT>::iterator;

    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const std::string& filename) : filename_(filename) {}

    virtual ~Database() {}

    virtual void sync() = 0;
    virtual void show_table(std::ostream&) = 0;
    virtual bool add(const PtrT&) = 0;
    virtual bool update(const PtrT&) = 0;

    void for_range(const std::function<void(PtrT&)>&);
    PtrT get_copy_by_id(std::int64_t);
    std::int64_t get_last_id() noexcept { return vec_.size(); }

protected:
    std::string filename_;
    std::string last_err_msg_;

    std::mutex mutex_vec_;

    iterator get_by_id(std::int64_t);
    void copy_sql_file();
    void send_query(const std::string&, int (*)(void*, int, char**, char**) = nullptr, void* = nullptr);

    std::vector<PtrT> vec_;
};

class Userbase : public Database<UserExtended>
{
public:
    using Ptr = std::shared_ptr<Userbase>;

    Userbase(const std::string&);

    bool add(const TgBot::User::Ptr&) override;
    bool update(const TgBot::User::Ptr&) override;

    void sync() override;
    void show_table(std::ostream&) override;
};

class Notifbase : public Database<Notification>
{
public:
    using Ptr = std::shared_ptr<Notifbase>;

    Notifbase(const std::string&);

    bool add(const Notification::Ptr&) override;
    bool update(const Notification::Ptr&) override;

    void sync() override;
    void show_table(std::ostream&) override;
};

template<typename T>
void Database<T>::send_query(const std::string& query, int (*callback)(void*, int, char**, char**), void* container)
{
    std::lock_guard<std::mutex> lock(mutex_sql_);

    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(filename_.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        last_err_msg_ = std::string("File '") + filename_ + "' is unavailable.";

        Logger::write(": ERROR : DATABASE : " + last_err_msg_);

        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    rc = sqlite3_exec(db, query.c_str(), callback, container, &err_msg);


    if(rc != SQLITE_OK)
    {
        last_err_msg_ =  err_msg;

        Logger::write(": ERROR : DATABASE : " + last_err_msg_ + " :: QUERY :: " + query);

        sqlite3_free(err_msg);
        sqlite3_close(db);
    }

    sqlite3_close(db);
}

template<typename T>
void Database<T>::copy_sql_file()
{
    std::lock_guard<std::mutex> lock(mutex_sql_); // Declaring a lock_guard with the same SQL mutex before calling this function leads to deadlock.
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
    Logger::write(": INFO : FILESYSTEM : File '" + filename_ + "' has been copied.");
}

template<typename T>
Database<T>::iterator Database<T>::get_by_id(std::int64_t id)
{
    return find_if(vec_.begin(), vec_.end(), [&id](const PtrT& x) { return x->id == id; });
}

template<typename T>
void Database<T>::for_range(const std::function<void(PtrT&)>& f)
{
    std::lock_guard<std::mutex> lock_vec(mutex_vec_);
    std::for_each(vec_.begin(), vec_.end(), f);
}

template<typename T>
Database<T>::PtrT Database<T>::get_copy_by_id(std::int64_t id)
{
    auto current_it = get_by_id(id);
    if(current_it == vec_.end())
        return nullptr;
    return PtrT(new T(*(*current_it)));
}

std::vector<TmExtended> extract_schedule(const std::string&, const std::string&);
