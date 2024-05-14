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
    std::lock_guard<std::mutex> lock(mutex_sql_);

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


    const char* query =
        "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, tg_id INTEGER, tg_uname TEXT, tg_fname TEXT, tg_lname TEXT, tg_langcode TEXT, tg_bot BOOLEAN, tg_prem BOOLEAN, tg_ATAM BOOLEAN, tg_CJG BOOLEAN, tg_CRAGM BOOLEAN, tg_SIQ BOOLEAN);"
        "SELECT * FROM users";

    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        rc = sqlite3_exec(db, query, extract_row, &users_vec_, &err_msg);
    }



    if(rc != SQLITE_OK)
    {
        last_err_msg_ = err_msg;

        logger_(": ERROR : DB : " + last_err_msg_, "./logs/log.log");

        sqlite3_free(err_msg);
        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    sqlite3_close(db);

    logger_(": INFO : DB : Database " + filename_ + " has been initialized.", "./logs/log.log");
}

void Database::copy_sql_file() const
{
    // Declaring a lock_guard with the same SQL mutex before calling this function leads to deadlock.
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
    logger_(": INFO : FILESYSTEM : Database " + filename_ + " has been copied.", "./logs/log.log");

}

bool Database::contains(const TgBot::User::Ptr& user)
{
    std::lock_guard<std::mutex> lock(mutex_vec_);

    auto comp = std::bind([](const TgBot::User::Ptr& x, const TgBot::User::Ptr& y) { return x->id == y->id; }, std::placeholders::_1, user); // originally binder1st; it transforms a binary predicate compare to a unary.
    auto existing_user_It = find_if(users_vec_.begin(), users_vec_.end(), comp);

    if(existing_user_It == users_vec_.end())
        return false;

    return true;
}



void Database::user_add(const TgBot::User::Ptr& user)
{
    // Make a backup of the previous version before saving!
    std::lock_guard<std::mutex> lock(mutex_sql_);

    try
    {
        copy_sql_file();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        last_err_msg_ = ex.what();
        logger_(": ERROR : FILESYSTEM : " + last_err_msg_, "./logs/log.log");

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


    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        users_vec_.push_back(user);
    }


    sqlite3_close(db);

    logger_(": INFO : DB : User [" + std::to_string(user->id) + "] " + user->firstName + " has been added.", "./logs/log.log");
}

void Database::user_update(const TgBot::User::Ptr& user)
{
    std::vector<TgBot::User::Ptr>::iterator existing_user_It;

    // VECTOR CONTEXT LOCK_GUARD
    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);

        // Searching for the user in the vector.

        auto comp = std::bind([](const TgBot::User::Ptr& x, const TgBot::User::Ptr& y) { return x->id == y->id; }, std::placeholders::_1, user); // originally binder1st; it transforms a binary predicate compare to a unary.
        existing_user_It = find_if(users_vec_.begin(), users_vec_.end(), comp);

        if(existing_user_It == users_vec_.end())
            return;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(user->username != (*existing_user_It)->username)
        {
            info_updated = true;
            user->username = (*existing_user_It)->username;
        }

        if(user->firstName != (*existing_user_It)->firstName)
        {
            info_updated = true;
            user->firstName = (*existing_user_It)->firstName;
        }

        if(user->lastName != (*existing_user_It)->lastName)
        {
            info_updated = true;
            user->lastName = (*existing_user_It)->lastName;
        }

        if(user->languageCode != (*existing_user_It)->languageCode)
        {
            info_updated = true;
            user->languageCode = (*existing_user_It)->languageCode;
        }

        if(user->isBot != (*existing_user_It)->isBot)
        {
            info_updated = true;
            user->isBot = (*existing_user_It)->isBot;
        }

        if(user->isPremium != (*existing_user_It)->isPremium)
        {
            info_updated = true;
            user->isPremium = (*existing_user_It)->isPremium;
        }

        if(user->addedToAttachmentMenu != (*existing_user_It)->addedToAttachmentMenu)
        {
            info_updated = true;
            user->addedToAttachmentMenu = (*existing_user_It)->addedToAttachmentMenu;
        }

        if(user->canJoinGroups != (*existing_user_It)->canJoinGroups)
        {
            info_updated = true;
            user->canJoinGroups = (*existing_user_It)->canJoinGroups;
        }

        if(user->canReadAllGroupMessages != (*existing_user_It)->canReadAllGroupMessages)
        {
            info_updated = true;
            user->canReadAllGroupMessages = (*existing_user_It)->canReadAllGroupMessages;
        }

        if(user->supportsInlineQueries != (*existing_user_It)->supportsInlineQueries)
        {
            info_updated = true;
            user->supportsInlineQueries = (*existing_user_It)->supportsInlineQueries;
        }

        if(!info_updated)
            return;
    }

    // SQL DB CONTEXT LOCK_GUARD
    {
        // Updating the entry in the SQLite DB.

        std::lock_guard<std::mutex> lock_sql(mutex_sql_);

        try
        {
            copy_sql_file();
        }
        catch(const boost::filesystem::filesystem_error& ex)
        {
            last_err_msg_ = ex.what();
            logger_(": ERROR : FILESYSTEM : " + last_err_msg_, "./logs/log.log");

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
            (std::string)"UPDATE users SET tg_uname='" + std::string(user->username)
            + std::string("', tg_fname='") + std::string(user->firstName)
            + std::string("', tg_lname='") + std::string(user->lastName)
            + std::string("', tg_langcode='")+ std::string(user->languageCode)
            + std::string("', tg_bot=") + std::string(user->isBot ? "TRUE" : "FALSE")
            + std::string(", tg_prem=") + std::string(user->isPremium ? "TRUE" : "FALSE")
            + std::string(", tg_ATAM=") + std::string(user->addedToAttachmentMenu ? "TRUE" : "FALSE")
            + std::string(", tg_CJG=") + std::string(user->canJoinGroups ? "TRUE" : "FALSE")
            + std::string(", tg_CRAGM=") + std::string(user->canReadAllGroupMessages ? "TRUE" : "FALSE")
            + std::string(", tg_SIQ=") + std::string(user->supportsInlineQueries ? "TRUE" : "FALSE")
            + std::string(" WHERE tg_id=") + std::to_string(user->id);

        rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);


        if(rc != SQLITE_OK)
        {
            last_err_msg_ =  err_msg;

            logger_(": ERROR : DB : " + last_err_msg_, "./logs/log.log");

            sqlite3_free(err_msg);
            sqlite3_close(db);

            throw Database::db_exception(last_err_msg_);
        }


        sqlite3_close(db);
    }

    logger_(": INFO : DB : User [" + std::to_string(user->id) + "] " + user->firstName + " has been updated.", "./logs/log.log");
}
