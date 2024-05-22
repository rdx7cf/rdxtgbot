#include "database.h"


///////////////////////
// AUX SECTION OPEN //
/////////////////////

static int extract_row(void* users, int colcount, char** columns, char** colnames)
{
    std::shared_ptr<UserExtended> user(new UserExtended);

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
    user->active_tasks_ = std::stoul(columns[12]);

    reinterpret_cast<std::vector<UserExtended::Ptr>*>(users)->push_back(user);

    return 0;
}

////////////////////////
// AUX SECTION CLOSE //
//////////////////////

Database::Database(const std::string& filename) : filename_(filename)
{
    std::lock_guard<std::mutex> lock(mutex_sql_);

    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(filename_.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        last_err_msg_ = std::string("An error occured while reading the file ") + filename_;

        Logger::write(": ERROR : DATABASE : " + last_err_msg_);

        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }


    const char* query =
        "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, tg_id INTEGER, tg_uname TEXT, tg_fname TEXT, tg_lname TEXT, tg_langcode TEXT, tg_bot BOOLEAN, tg_prem BOOLEAN, tg_ATAM BOOLEAN, tg_CJG BOOLEAN, tg_CRAGM BOOLEAN, tg_SIQ BOOLEAN, tg_activetasks INTEGER);"
        "SELECT * FROM users";

    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        rc = sqlite3_exec(db, query, extract_row, &users_vec_, &err_msg);
    }



    if(rc != SQLITE_OK)
    {
        last_err_msg_ = err_msg;

        Logger::write(": ERROR : DATABASE : " + last_err_msg_);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    sqlite3_close(db);

    Logger::write(": INFO : DATABASE : Database " + filename_ + " has been initialized.");
}

void Database::copy_sql_file() const
{
    // Declaring a lock_guard with the same SQL mutex before calling this function leads to deadlock.
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
    Logger::write(": INFO : FILESYSTEM : Database " + filename_ + " has been copied.");

}

bool Database::contains(const TgBot::User::Ptr& user)
{
    std::lock_guard<std::mutex> lock(mutex_vec_);

    auto comp = std::bind([](const UserExtended::Ptr& x, const TgBot::User::Ptr& y) { return x->id == y->id; }, std::placeholders::_1, user); // originally binder1st; it transforms a binary predicate compare to a unary.
    auto existing_user_It = find_if(users_vec_.begin(), users_vec_.end(), comp);

    if(existing_user_It == users_vec_.end())
        return false;

    return true;
}



void Database::user_add(const UserExtended::Ptr& user)
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
        Logger::write(": ERROR : FILESYSTEM : " + last_err_msg_);

        throw ex;
    }



    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(filename_.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        last_err_msg_ = std::string("An error occured while reading the file ") + filename_;

        Logger::write(": ERROR : DATABASE : " + last_err_msg_);

        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }


    std::string query =
        (std::string)"INSERT INTO users (tg_id, tg_uname, tg_fname, tg_lname, tg_langcode, tg_bot, tg_prem, tg_ATAM, tg_CJG, tg_CRAGM, tg_SIQ, tg_activetasks) VALUES ("
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
        + std::string(", ")
        + std::to_string(user->active_tasks_.to_ulong())
        + std::string(");");

    rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);


    if(rc != SQLITE_OK)
    {
        last_err_msg_ =  err_msg;

        Logger::write(": ERROR : DB : " + last_err_msg_);

        sqlite3_free(err_msg);
        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }


    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        users_vec_.push_back(user);
    }


    sqlite3_close(db);

    Logger::write(": INFO : DB : User [" + std::to_string(user->id) + "] " + user->firstName + " has been added.");
}

void Database::user_update(const TgBot::User::Ptr& user)
{
    std::vector<UserExtended::Ptr>::iterator existing_user_It;

    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);

        // Searching for the user in the vector.

        auto comp = std::bind([](const UserExtended::Ptr& x, const TgBot::User::Ptr& y) { return x->id == y->id; }, std::placeholders::_1, user); // originally binder1st; it transforms a binary predicate compare to a unary.
        existing_user_It = find_if(users_vec_.begin(), users_vec_.end(), comp);

        if(existing_user_It == users_vec_.end())
            return;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(user->username != (*existing_user_It)->username)
        {
            info_updated = true;
            (*existing_user_It)->username = user->username;
        }

        if(user->firstName != (*existing_user_It)->firstName)
        {
            info_updated = true;
            (*existing_user_It)->firstName = user->firstName;
        }

        if(user->lastName != (*existing_user_It)->lastName)
        {
            info_updated = true;
            (*existing_user_It)->lastName = user->lastName;
        }

        if(user->languageCode != (*existing_user_It)->languageCode)
        {
            info_updated = true;
            (*existing_user_It)->languageCode = user->languageCode;
        }

        if(user->isBot != (*existing_user_It)->isBot)
        {
            info_updated = true;
            (*existing_user_It)->isBot = user->isBot;
        }

        if(user->isPremium != (*existing_user_It)->isPremium)
        {
            info_updated = true;
            (*existing_user_It)->isPremium = user->isPremium;
        }

        if(user->addedToAttachmentMenu != (*existing_user_It)->addedToAttachmentMenu)
        {
            info_updated = true;
            (*existing_user_It)->addedToAttachmentMenu = user->addedToAttachmentMenu;
        }

        if(user->canJoinGroups != (*existing_user_It)->canJoinGroups)
        {
            info_updated = true;
            (*existing_user_It)->canJoinGroups = user->canJoinGroups;
        }

        if(user->canReadAllGroupMessages != (*existing_user_It)->canReadAllGroupMessages)
        {
            info_updated = true;
            (*existing_user_It)->canReadAllGroupMessages = user->canReadAllGroupMessages;
        }

        if(user->supportsInlineQueries != (*existing_user_It)->supportsInlineQueries)
        {
            info_updated = true;
            (*existing_user_It)->supportsInlineQueries = user->supportsInlineQueries;
        }

        if(!info_updated)
            return;
    }
    Logger::write(": INFO : DATABASE : User [" + std::to_string(user->id) + "] " + user->firstName + " has been updated.");
}

void Database::sync()
{
    std::lock_guard<std::mutex> lock_sql(mutex_sql_);

    try
    {
        copy_sql_file();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        last_err_msg_ = ex.what();
        Logger::write(": ERROR : FILESYSTEM : " + last_err_msg_);

        throw ex;
    }

    char* err_msg = nullptr;

    sqlite3* db;
    int rc = sqlite3_open(filename_.c_str(), &db);

    if(rc != SQLITE_OK)
    {
        last_err_msg_ = std::string("An error occured while reading the file ") + filename_;

        Logger::write(": ERROR : DB : " + last_err_msg_);

        sqlite3_close(db);

        throw Database::db_exception(last_err_msg_);
    }

    std::string query;

    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);
        std::for_each(users_vec_.begin(), users_vec_.end(), [&](const UserExtended::Ptr& user)
        {
            query =
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
                    + std::string(", tg_activetasks=") + std::to_string(user->active_tasks_.to_ulong())
                    + std::string(" WHERE tg_id=") + std::to_string(user->id);

            rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &err_msg);


            if(rc != SQLITE_OK)
            {
                last_err_msg_ =  err_msg;

                Logger::write(": ERROR : DB : " + last_err_msg_);

                sqlite3_free(err_msg);
                sqlite3_close(db);

                throw Database::db_exception(last_err_msg_);
            }
        });
    }


    sqlite3_close(db);
}
