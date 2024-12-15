#ifndef TABLE_H
#define TABLE_H

#include "SQLFile.h"
#include "UserExtended.h"
#include "Notification.h"
#include "VPS.h"

template<typename T>
class Table
{
public:
    using SptrT = std::shared_ptr<T>;
    using Iterator = std::vector<SptrT>::iterator;
    using ConstIterator = std::vector<SptrT>::const_iterator;
    using SptrF = std::shared_ptr<SQLFile>;


    class TableException : public std::runtime_error
    {
    public:
        TableException(const std::string& what_arg) : std::runtime_error(what_arg) {}
    };

    Table(const SptrF& file, int interval = -1) : file_(file), interval_(interval) {}
    virtual ~Table() {}

    void autoSync(std::stop_token) const;

    void forRange(const std::function<void(SptrT&)>&);
    void forRange(const std::function<void(const SptrT&)>&) const;

    SptrT getCopyBy(const std::function<bool(const SptrT&)>&) const noexcept;

    std::int64_t getLastId() const noexcept { return vec_.size(); }

    virtual bool add(const SptrT&);
    virtual bool update(const SptrT&) noexcept = 0;
    virtual void sync() const = 0;
    virtual void showTable(std::ostream&) const noexcept = 0;


protected:
    SptrF file_;
    int interval_;
    mutable std::mutex mtx_vec_;
    std::vector<SptrT> vec_;

    bool updateNeeded(const SptrT&, const SptrT&);
    SptrT getBy(const std::function<bool(SptrT&)>& f);
    SptrT getBy(const std::function<bool(const SptrT&)>& f) const;
};

class UserTable : public Table<UserExtended>
{
public:
    using Ptr = std::shared_ptr<UserTable>;

    UserTable(const Table<UserExtended>::SptrF&, int = -1);

    bool add(const UserExtended::Ptr&) override;
    bool update(const UserExtended::Ptr&) noexcept override;

    void sync() const override;
    void showTable(std::ostream&) const noexcept override;
};

class NotificationTable : public Table<Notification>
{
public:
    using Ptr = std::shared_ptr<NotificationTable>;

    NotificationTable(const Table<Notification>::SptrF&, int = -1);

    bool add(const Notification::Ptr&) override;
    bool update(const Notification::Ptr&) noexcept override;

    void sync() const override;
    void showTable(std::ostream&) const noexcept override;
};

class VPSTable : public Table<VPS>
{
public:
    using Ptr = std::shared_ptr<VPSTable>;

    VPSTable(const Table<VPS>::SptrF&, int = -1);

    bool add(const VPS::Ptr&) override;
    bool update(const VPS::Ptr&) noexcept override;

    void sync() const override;
    void showTable(std::ostream&) const noexcept override;
};

template<typename T>
void Table<T>::autoSync(std::stop_token tok) const
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
bool Table<T>::add(const SptrT& entry)
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    auto existing = getBy([&entry](const SptrT& e)
    {
        return *entry == *e;
    });

    if(!existing)
    {
        vec_.push_back(entry);
        return true;
    }

    return false;
}

template<typename T>
Table<T>::SptrT Table<T>::getBy(const std::function<bool(SptrT&)>& f)
{
    return *std::find_if(vec_.begin(), vec_.end(), f);
}

template<typename T>
Table<T>::SptrT Table<T>::getBy(const std::function<bool(const SptrT&)>& f) const
{
    return *std::find_if(vec_.cbegin(), vec_.cend(), f);
}

template<typename T>
void Table<T>::forRange(const std::function<void(SptrT&)>& f)
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    std::for_each(vec_.begin(), vec_.end(), f);
}

template<typename T>
void Table<T>::forRange(const std::function<void(const SptrT&)>& f) const
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);
    std::for_each(vec_.cbegin(), vec_.cend(), f);
}

// auto f = [&id](const SptrT& entry) { return entry->id == id; };
template<typename T>
Table<T>::SptrT Table<T>::getCopyBy(const std::function<bool(const SptrT&)>& f) const noexcept
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);

    auto current = getBy(f);

    if(!current)
        return nullptr;

    return std::make_shared<T>(*current);
}


std::vector<TmExtended> extractSchedule(const std::string&, const std::string&) noexcept;

#endif // TABLE_H
