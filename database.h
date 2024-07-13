#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <memory>

#include <tgbot/tgbot.h>
#include <boost/filesystem.hpp>
#include <sqlite3.h>

#include "logger.h"
#include "userextended.h"
#include "notification.h"
#include "ctime++.h"
#include "sqlfile.h"

template<typename T>
class Database
{
public:
    using PtrT = std::shared_ptr<T>;
    using PtrF = std::shared_ptr<SQLFile>;
    using iterator = std::vector<PtrT>::iterator;
    using const_iterator = std::vector<PtrT>::const_iterator;

    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const PtrF& file, int interval = -1) : file_(file), interval_(interval) {}
    virtual ~Database() {}

    virtual void sync() const = 0;
    virtual void show_table(std::ostream&) const noexcept = 0;
    virtual bool add(const PtrT&) = 0;
    virtual bool update(const PtrT&) noexcept = 0;

    void auto_sync(std::stop_token) const;
    void for_range(const std::function<void(PtrT&)>&);
    void for_range(const std::function<void(const PtrT&)>&) const;
    PtrT get_copy_by_id(std::int64_t) const noexcept;
    std::int64_t get_last_id() const noexcept { return vec_.size(); }

protected:
    mutable std::mutex mtx_vec_;
    PtrF file_;
    std::vector<PtrT> vec_;
    int interval_;

    iterator get_by_id(std::int64_t) noexcept;
    const_iterator get_by_id(std::int64_t) const noexcept;
};

class Userbase : public Database<UserExtended>
{
public:
    using Ptr = std::shared_ptr<Userbase>;

    Userbase(const Database<UserExtended>::PtrF&, int = -1);

    bool add(const UserExtended::Ptr&) override;
    bool update(const UserExtended::Ptr&) noexcept override;

    void sync() const override;
    void show_table(std::ostream&) const noexcept override;
};

class Notifbase : public Database<Notification>
{
public:
    using Ptr = std::shared_ptr<Notifbase>;

    Notifbase(const Database<Notification>::PtrF&, int = -1);

    bool add(const Notification::Ptr&) override;
    bool update(const Notification::Ptr&) noexcept override;

    void sync() const override;
    void show_table(std::ostream&) const noexcept override;
};

template<typename T>
void Database<T>::auto_sync(std::stop_token tok) const
{
    if(interval_ < 0)
        return;

    Logger::write(": INFO : DATABASE : Loop sync has been started.");

    while(!tok.stop_requested())
    {
        sync();
        Logger::write(": INFO : DATABASE : Next sync will be in: " + std::to_string(interval_) + " seconds.");
        for(std::int32_t wait = 0; wait < interval_ && !tok.stop_requested(); ++wait )
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Logger::write(": INFO : DATABASE : Loop sync has been stopped.");
}

template<typename T>
Database<T>::iterator Database<T>::get_by_id(std::int64_t id) noexcept
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    return std::find_if(vec_.begin(), vec_.end(), [&id](const PtrT& x) { return x->id == id; });
}

template<typename T>
Database<T>::const_iterator Database<T>::get_by_id(std::int64_t id) const noexcept
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    return std::find_if(vec_.cbegin(), vec_.cend(), [&id](const PtrT& x) { return x->id == id; });
}

template<typename T>
void Database<T>::for_range(const std::function<void(PtrT&)>& f)
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    std::for_each(vec_.begin(), vec_.end(), f);
}

template<typename T>
void Database<T>::for_range(const std::function<void(const PtrT&)>& f) const
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    std::for_each(vec_.cbegin(), vec_.cend(), f);
}

template<typename T>
Database<T>::PtrT Database<T>::get_copy_by_id(std::int64_t id) const noexcept
{
    auto current_it = get_by_id(id);
    if(current_it == vec_.cend())
        return nullptr;
    return PtrT(new T(*(*current_it)));
}

std::vector<TmExtended> extract_schedule(const std::string&, const std::string&) noexcept;
