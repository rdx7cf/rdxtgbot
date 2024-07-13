#include "sqlfile.h"

SQLFile::SQLFile(const std::string& filename, sqlite3* connection) : filename_(filename), connection_(connection)
{
    if(connection_ == nullptr)
    {
        int rc = sqlite3_open(filename_.c_str(), &connection_);

        if(rc != SQLITE_OK)
        {
            std::string err_msg_ = ": ERROR : DATABASE : File '" + filename_ + "' is unavailable.";

            Logger::write(err_msg_);

            throw sqlfile_exception(err_msg_);
        }
    }
}

SQLFile::~SQLFile()
{
    sqlite3_close(connection_);
}

void SQLFile::send_query(const std::string& query, int (*callback)(void*, int, char**, char**), void* container) const noexcept
{
    std::lock_guard<std::mutex> lock(mtx_sql_);

    int rc = sqlite3_exec(connection_, query.c_str(), callback, container, &err_msg_);

    if(rc != SQLITE_OK)
    {
        Logger::write(std::string(": ERROR : DATABASE : ") + err_msg_ + " :: QUERY :: " + query);

        sqlite3_free(err_msg_);
    }
}

void SQLFile::backup() const
{
    std::lock_guard<std::mutex> lock(mtx_sql_);
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
    Logger::write(": INFO : FILESYSTEM : File '" + filename_ + "' has been copied.");
}

void SQLFile::auto_backup(std::stop_token tok, std::int32_t seconds) const noexcept
{
    if(seconds < 0)
        return;

    Logger::write(": INFO : BOT : Loop sync has been started.");

    while(!tok.stop_requested())
    {
        try
        {
            backup();
        }
        catch(const std::exception& e)
        {
            Logger::write(": INFO : BOT : An error occured while backing up the SQL File.");
        }
        Logger::write(": INFO : BOT : Next backup will be in: " + std::to_string(seconds) + " seconds.");
        for(std::int32_t wait = 0; wait < seconds && !tok.stop_requested(); ++wait )
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Logger::write(": INFO : BOT : Loop sync has been stopped.");
}
