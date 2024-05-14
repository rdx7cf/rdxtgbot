#include "database.h"

//
// AUX SECTION OPEN
//

static int extract_row(void* users, int colcount, char** columns, char** colnames)
{
    std::shared_ptr<TgBot::User> user(new TgBot::User);

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

    reinterpret_cast<std::vector<TgBot::User::Ptr>*>(users)->push_back(user);

    return 0;
}

////////////////////////
// AUX SECTION CLOSE //
//////////////////////

Database::Database(const std::string& filename, std::function<void(const std::string&, const std::string&)> logger) : filename_(filename), logger_(logger)
{
    std::lock_guard<std::mutex> lock(mutex_db_);

    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(filename_.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        last_err_msg_ = std::string("An error occured while reading the file ") + filename_;

        logger_(": ERROR : DB : " + last_err_msg_, "./logs/log.log");

        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }


    const char* query = "SELECT * FROM users;";

    rc = sqlite3_exec(db, query, extract_row, &users_vec_, &err_msg);


    if(rc != SQLITE_OK)
    {
        last_err_msg_ = err_msg;

        logger_(": ERROR : DB : " + last_err_msg_, "./logs/log.log");

        sqlite3_free(err_msg);
        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    sqlite3_close(db);
}

void Database::copy() const
{
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
}

bool Database::contains(const TgBot::User::Ptr& user)
{
    std::lock_guard<std::mutex> lock(mutex_db_);

    auto comp = std::bind([](const TgBot::User::Ptr& x, const TgBot::User::Ptr& y) { return x->id == y->id; }, std::placeholders::_1, user); // originally binder1st; it transforms a binary predicate compare to a unary.
    auto existing_user_It = find_if(users_vec_.begin(), users_vec_.end(), comp);

    if(existing_user_It == users_vec_.end())
        return false;

    return true;
}

void Database::user_add(const TgBot::User::Ptr& user)
{
    // Make a backup of the previous version before saving!

    std::lock_guard<std::mutex> lock(mutex_db_);

    try
    {
        copy();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        last_err_msg_ = std::string(": ERROR : FILESYSTEM : ") + ex.what() + "'.\n";

        throw ex;
    }


    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(filename_.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        last_err_msg_ = std::string("An error occured while reading the file ") + filename_;

        logger_(": ERROR : DB : " + last_err_msg_, "./logs/log.log");

        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }


    std::string query =
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
        + std::string(");");

    rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);


    if(rc != SQLITE_OK)
    {
        last_err_msg_ =  err_msg;

        logger_(": ERROR : DB : " + last_err_msg_, "./logs/log.log");

        sqlite3_free(err_msg);
        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    users_vec_.push_back(user);

    sqlite3_close(db);
}
