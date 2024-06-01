#include "database.h"

template<class T>
void Database<T>::send_query(const std::string& query, int (*callback)(void*, int, char**, char**), void* container)
{
    std::lock_guard<std::mutex> lock(mutex_sql_);

    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(filename_.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        last_err_msg_ = std::string("FILE UNAVAILABLE: '") + filename_ + "'";

        Logger::write(": ERROR : BAS : " + last_err_msg_);

        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    rc = sqlite3_exec(db, query.c_str(), callback, container, &err_msg);


    if(rc != SQLITE_OK)
    {
        last_err_msg_ =  err_msg;

        Logger::write(": ERROR : BAS : " + last_err_msg_);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    sqlite3_close(db);
}

template<class T>
void Database<T>::copy_sql_file()
{
    std::lock_guard<std::mutex> lock(mutex_sql_); // Declaring a lock_guard with the same SQL mutex before calling this function leads to deadlock.
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
    Logger::write(": INFO : FIL : '" + filename_ + "' COPIED.");
}

template<class T>
bool Database<T>::contains(const dataPtr& entry)
{
    std::lock_guard<std::mutex> lock(mutex_vec_);

    auto existing_user_it = find_if(vec_.begin(), vec_.end(), [&entry](const dataPtr& x) { return x->id == entry->id; });

    if(existing_user_it == vec_.end())
        return false;

    return true;
}

template<class T>
bool Database<T>::contains(const std::int64_t& id)
{
    std::lock_guard<std::mutex> lock(mutex_vec_);

    auto existing_user_it = find_if(vec_.begin(), vec_.end(), [&id](const dataPtr& x) { return x->id == id; });

    if(existing_user_it == vec_.end())
        return false;

    return true;
}
