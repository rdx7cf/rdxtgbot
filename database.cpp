#include "database.h"


// AUX SECTION OPEN

static int extract_row(void* users, int colcount, char** columns, char** colnames)
{
    TgBot::User* user = new TgBot::User;

    user->id = std::stol(columns[1]);
    user->username = columns[2];
    user->firstName = columns[3];
    user->lastName = columns[4];
    user->languageCode = columns[5];
    user->isBot = columns[6];
    user->isPremium = columns[7];
    user->addedToAttachmentMenu = columns[8];
    user->canJoinGroups = columns[9];
    user->canReadAllGroupMessages = columns[10];
    user->supportsInlineQueries = columns[11];

    reinterpret_cast<std::vector<TgBot::User::Ptr>*>(users)->push_back(TgBot::User::Ptr(user));

    return 0;
}

static void make_backup(const std::string& database)
{
    boost::filesystem::copy_file(database, database + ".bak+" + std::to_string(rand()), boost::filesystem::copy_options::overwrite_existing);
}

static void remove_oldest()
{

}

// AUX SECTION CLOSED

int db_readOnStart(const char* database, std::vector<TgBot::User::Ptr>& users)
{
    std::lock_guard<std::mutex> lock(GLOBAL::mutex_db);

    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(database, &db);

    if(rc != SQLITE_OK)
    {
        std::string log_message = std::string(": ERROR : SQLite : An error occured while reading the file ") + database + "'.\n";

        to_filelog(log_message);

        sqlite3_close(db);
        return rc;
    }


    const char* query = "SELECT * FROM users;";

    rc = sqlite3_exec(db, query, extract_row, &users, &err_msg);


    if(rc != SQLITE_OK)
    {
        std::string log_message = std::string(": ERROR : SQLite : ") + err_msg + "'.\n";

        to_filelog(log_message);

        sqlite3_free(err_msg);
        sqlite3_close(db);
        return rc;
    }

    sqlite3_close(db);
    return 0;
}

int db_save(const char* database, std::vector<TgBot::User::Ptr>& users)
{
    // Make a backup of the previous version before saving!

    std::lock_guard<std::mutex> lock(GLOBAL::mutex_db);

    try
    {
        make_backup(database);
    }
    catch(const std::exception& ex)
    {
        std::string log_message = std::string(": ERROR : FILESYSTEM : ") + ex.what() + "'.\n";

        to_filelog(log_message);
        throw ex;
    }


    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(database, &db);

    if(rc != SQLITE_OK)
    {
        std::string log_message = std::string(": ERROR : SQLite : An error occured while reading the file ") + database + "'.\n";

        to_filelog(log_message);
        sqlite3_close(db);
        throw sqlite3_exception("SQLite : An error has occured while reading the file.");
    }


    std::string query =   "DROP TABLE IF EXISTS users;"
                          "CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, tg_id INTEGER, tg_uname TEXT, tg_fname TEXT, tg_lname TEXT, tg_langcode TEXT, tg_bot BOOLEAN, tg_prem BOOLEAN, tg_ATAM BOOLEAN, tg_CJG BOOLEAN, tg_CRAGM BOOLEAN, tg_SIQ BOOLEAN);";

    rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);


    if(rc != SQLITE_OK)
    {
        std::string log_message = std::string(": ERROR : SQLite : ") + err_msg + "'.\n";

        to_filelog(log_message);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        throw sqlite3_exception("SQLite : An error has occured while sending query, check the log file for details.");
    }



    std::for_each(users.begin(), users.end(), [&query, &db, &err_msg](const TgBot::User::Ptr& user)
    {
        query =
            (std::string)"INSERT INTO users (tg_id, tg_uname, tg_fname, tg_lname, tg_langcode, tg_bot, tg_prem, tg_ATAM, tg_CJG, tg_CRAGM, tg_SIQ) VALUES ("
                + std::to_string(user->id)
                + std::string(", '")
                + std::string(user->username)
                + std::string("', '")
                + std::string(user->firstName)
                + std::string("', '")
                + std::string(user->lastName)
                + std::string("', '")
                + std::string(user->languageCode)
                + std::string("', ")
                + std::string(user->isBot ? "TRUE" : "FALSE")
                + std::string(", ")
                + std::string(user->isPremium ? "TRUE" : "FALSE")
                + std::string(", ")
                + std::string(user->addedToAttachmentMenu ? "TRUE" : "FALSE")
                + std::string(", ")
                + std::string(user->canJoinGroups ? "TRUE" : "FALSE")
                + std::string(", ")
                + std::string(user->canReadAllGroupMessages ? "TRUE" : "FALSE")
                + std::string(", ")
                + std::string(user->supportsInlineQueries ? "TRUE" : "FALSE")
                + std::string(");")
            ;
    });

    sqlite3_close(db);

    return 0;
}

void db_sync(const char* database, std::vector<TgBot::User::Ptr>& users, unsigned)
{
    // It'll be calling db_save for interaction with the database; the only purpose of this function is organizing sync loop.
}
