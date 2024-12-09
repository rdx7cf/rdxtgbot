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
#include "vps.h"

template<typename T>
class Database
{
public:
    using sPtrT = std::shared_ptr<T>;
    using uPtrT = std::unique_ptr<T>;
    using sPtrF = std::shared_ptr<SQLFile>;
    using iterator = std::vector<sPtrT>::iterator;
    using const_iterator = std::vector<sPtrT>::const_iterator;

    class db_exception : public std::runtime_error
    {
    public:
        db_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Database(const sPtrF& file, int interval = -1) : file_(file), interval_(interval) {}
    virtual ~Database() {}

    virtual void sync() const = 0;
    virtual void show_table(std::ostream&) const noexcept = 0;
    virtual bool add(const sPtrT&) = 0;
    virtual bool update(const sPtrT&) noexcept = 0;

    void auto_sync(std::stop_token) const;
    void for_range(const std::function<void(sPtrT&)>&);
    void for_range(const std::function<void(const sPtrT&)>&) const;

    sPtrT get_copy_by(const std::function<bool(const sPtrT&)>&) const noexcept;

    std::int64_t get_last_id() const noexcept { return vec_.size(); }

protected:
    mutable std::mutex mtx_vec_;
    sPtrF file_;
    std::vector<sPtrT> vec_;
    int interval_;

    iterator find_if(const std::function<bool(sPtrT&)>&);
    const_iterator find_if(const std::function<bool(const sPtrT&)>&) const;
};

class Userbase : public Database<UserExtended>
{
public:
    using Ptr = std::shared_ptr<Userbase>;

    Userbase(const Database<UserExtended>::sPtrF&, int = -1);

    bool add(const UserExtended::Ptr&) override;
    bool update(const UserExtended::Ptr&) noexcept override;

    void sync() const override;
    void show_table(std::ostream&) const noexcept override;
};

class Notifbase : public Database<Notification>
{
public:
    using Ptr = std::shared_ptr<Notifbase>;

    Notifbase(const Database<Notification>::sPtrF&, int = -1);

    bool add(const Notification::Ptr&) override;
    bool update(const Notification::Ptr&) noexcept override;

    void sync() const override;
    void show_table(std::ostream&) const noexcept override;
};

class VPSbase : public Database<VPS>
{
public:
    using Ptr = std::shared_ptr<VPSbase>;

    VPSbase(const Database<VPS>::sPtrF&, int = -1);

    bool add(const VPS::Ptr&) override;
    bool update(const VPS::Ptr&) noexcept override;

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
Database<T>::iterator Database<T>::find_if(const std::function<bool(sPtrT&)>& f)
{
    return std::find_if(vec_.begin(), vec_.end(), f);
}

template<typename T>
Database<T>::const_iterator Database<T>::find_if(const std::function<bool(const sPtrT&)>& f) const
{
    return std::find_if(vec_.begin(), vec_.end(), f);
}

template<typename T>
void Database<T>::for_range(const std::function<void(sPtrT&)>& f)
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    std::for_each(vec_.begin(), vec_.end(), f);
}

template<typename T>
void Database<T>::for_range(const std::function<void(const sPtrT&)>& f) const
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    std::for_each(vec_.cbegin(), vec_.cend(), f);
}

// auto f = [&id](const sPtrT& entry) { return entry->id == id; };
template<typename T>
Database<T>::sPtrT Database<T>::get_copy_by(const std::function<bool(const sPtrT&)>& f) const noexcept
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);

    auto current_it = find_if(f);

    if(current_it == vec_.cend())
        return nullptr;

    return std::make_shared<T>(*(*current_it));
}

std::vector<TmExtended> extract_schedule(const std::string&, const std::string&) noexcept;
