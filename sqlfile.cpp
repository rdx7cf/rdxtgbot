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

void SQLFile::send_query(const std::string& query, int (*callback)(void*, int, char**, char**), void* container)
{
    std::lock_guard<std::mutex> lock(mtx_sql_);

    try
    {
        backup();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        Logger::write(std::string(": ERROR : FILESYSTEM : ") + ex.what());

        throw ex;
    }

    int rc = sqlite3_exec(connection_, query.c_str(), callback, container, &err_msg_);

    if(rc != SQLITE_OK)
    {
        Logger::write(std::string(": ERROR : DATABASE : ") + err_msg_ + " :: QUERY :: " + query);

        sqlite3_free(err_msg_);
    }
}

void SQLFile::backup()
{
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
    Logger::write(": INFO : FILESYSTEM : File '" + filename_ + "' has been copied.");
}
