#include "database.h"

///////////////////////
// AUX SECTION OPEN //
/////////////////////

static int extract_user(void* users, int colcount, char** columns, char** colnames)
{
    UserExtended::Ptr user(new UserExtended);

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
    user->activeTasks = std::stoul(columns[12]);

    reinterpret_cast<std::vector<UserExtended::Ptr>*>(users)->push_back(user);

    return 0;
}

static int extract_ad(void* ads, int colcount, char** columns, char** colnames)
{
    Ad::Ptr ad(new Ad);

    ad->id = std::stol(columns[0]);
    ad->owner = columns[1];
    ad->text = columns[2];
    ad->expiring_on = std::stol(columns[3]);

    reinterpret_cast<std::vector<Ad::Ptr>*>(ads)->push_back(ad);

    return 0;
}

////////////////////////
// AUX SECTION CLOSE //
//////////////////////

template<>
Database<UserExtended>::Database(const std::string& filename) : filename_(filename)
{
    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        send_query
                (
                    "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, tg_id INTEGER, tg_uname TEXT, tg_fname TEXT, tg_lname TEXT, tg_langcode TEXT, tg_bot BOOLEAN, tg_prem BOOLEAN, tg_ATAM BOOLEAN, tg_CJG BOOLEAN, tg_CRAGM BOOLEAN, tg_SIQ BOOLEAN, tg_activetasks INTEGER);"
                    "SELECT * FROM users",
                    extract_user,
                    &vec_
                );

    }

    Logger::write(": INFO : BAS : USR : INITIALIZED.");
}

template<>
Database<Ad>::Database(const std::string& filename) : filename_(filename)
{
    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        send_query
                (
                    "CREATE TABLE IF NOT EXISTS ads (id INTEGER PRIMARY KEY AUTOINCREMENT, owner TEXT, text TEXT, active BOOLEAN, expiring_on INTEGER);"
                    "SELECT * FROM ads",
                    extract_ad,
                    &vec_
                );
    }

    Logger::write(": INFO : BAS : USR : INITIALIZED.");
}

template<>
void Database<UserExtended>::add(const dataPtr& entry)
{
    try
    {
        copy_sql_file();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        last_err_msg_ = ex.what();
        Logger::write(": ERROR : FS : " + last_err_msg_);

        throw ex;
    }


    send_query(
        (std::string)"INSERT INTO users (tg_id, tg_uname, tg_fname, tg_lname, tg_langcode, tg_bot, tg_prem, tg_ATAM, tg_CJG, tg_CRAGM, tg_SIQ, tg_activetasks) VALUES ("
        + std::to_string(entry->id)
        + std::string(", '")
        + std::string(entry->username)
        + std::string("', '")
        + std::string(entry->firstName)
        + std::string("', '")
        + std::string(entry->lastName)
        + std::string("', '")
        + std::string(entry->languageCode)
        + std::string("', ")
        + std::string(entry->isBot ? "TRUE" : "FALSE")
        + std::string(", ")
        + std::string(entry->isPremium ? "TRUE" : "FALSE")
        + std::string(", ")
        + std::string(entry->addedToAttachmentMenu ? "TRUE" : "FALSE")
        + std::string(", ")
        + std::string(entry->canJoinGroups ? "TRUE" : "FALSE")
        + std::string(", ")
        + std::string(entry->canReadAllGroupMessages ? "TRUE" : "FALSE")
        + std::string(", ")
        + std::string(entry->supportsInlineQueries ? "TRUE" : "FALSE")
        + std::string(", ")
        + std::to_string(entry->activeTasks.to_ulong())
        + std::string(");"));

    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        vec_.push_back(entry);
    }

    Logger::write(": INFO : BAS : USR : [" + std::to_string(entry->id) + "] [" + entry->firstName + "] ADDED.");
}

template<>
void Database<Ad>::add(const dataPtr& entry)
{
    try
    {
        copy_sql_file();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        last_err_msg_ = ex.what();
        Logger::write(": ERROR : FS : " + last_err_msg_);

        throw ex;
    }

    send_query(
        (std::string)"INSERT INTO ads (owner, text, active, expiring_on) VALUES ("
        + std::string(entry->owner)
        + std::string("', '")
        + std::string(entry->text)
        + std::string("', ")
        + std::string(entry->active ? "TRUE" : "FALSE")
        + std::string(", ")
        + std::to_string(entry->expiring_on)
        + std::string(");"));

    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        vec_.push_back(entry);
    }

    Logger::write(": INFO : BAS : ADS : [" + std::to_string(entry->id) + "] [" + entry->owner + "] ADDED.");
}

template<>
void Database<UserExtended>::update(const dataPtr& entry)
{
    std::vector<dataPtr>::iterator existing_user_it;

    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);

        // Searching for the user in the vector.

        existing_user_it = find_if(vec_.begin(), vec_.end(), [&entry](const dataPtr& x) { return x->id == entry->id; });

        if(existing_user_it == vec_.end())
            return;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(entry->username != (*existing_user_it)->username)
        {
            info_updated = true;
            (*existing_user_it)->username = entry->username;
        }

        if(entry->firstName != (*existing_user_it)->firstName)
        {
            info_updated = true;
            (*existing_user_it)->firstName = entry->firstName;
        }

        if(entry->lastName != (*existing_user_it)->lastName)
        {
            info_updated = true;
            (*existing_user_it)->lastName = entry->lastName;
        }

        if(entry->languageCode != (*existing_user_it)->languageCode)
        {
            info_updated = true;
            (*existing_user_it)->languageCode = entry->languageCode;
        }

        if(entry->isBot != (*existing_user_it)->isBot)
        {
            info_updated = true;
            (*existing_user_it)->isBot = entry->isBot;
        }

        if(entry->isPremium != (*existing_user_it)->isPremium)
        {
            info_updated = true;
            (*existing_user_it)->isPremium = entry->isPremium;
        }

        if(entry->addedToAttachmentMenu != (*existing_user_it)->addedToAttachmentMenu)
        {
            info_updated = true;
            (*existing_user_it)->addedToAttachmentMenu = entry->addedToAttachmentMenu;
        }

        if(entry->canJoinGroups != (*existing_user_it)->canJoinGroups)
        {
            info_updated = true;
            (*existing_user_it)->canJoinGroups = entry->canJoinGroups;
        }

        if(entry->canReadAllGroupMessages != (*existing_user_it)->canReadAllGroupMessages)
        {
            info_updated = true;
            (*existing_user_it)->canReadAllGroupMessages = entry->canReadAllGroupMessages;
        }

        if(entry->supportsInlineQueries != (*existing_user_it)->supportsInlineQueries)
        {
            info_updated = true;
            (*existing_user_it)->supportsInlineQueries = entry->supportsInlineQueries;
        }

        if(!info_updated)
            return;
    }
    Logger::write(": INFO : BAS : USR : [" + std::to_string(entry->id) + "] [" + entry->firstName + "] UPDATED.");
}

template<>
void Database<Ad>::update(const dataPtr& entry)
{
    std::vector<dataPtr>::iterator existing_ad_it;

    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);

        // Searching for the ad in the vector.

        existing_ad_it = find_if(vec_.begin(), vec_.end(), [&entry](const dataPtr& x) { return x->id == entry->id; });

        if(existing_ad_it == vec_.end())
            return;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(entry->owner != (*existing_ad_it)->owner)
        {
            info_updated = true;
            (*existing_ad_it)->owner = entry->owner;
        }

        if(entry->text != (*existing_ad_it)->text)
        {
            info_updated = true;
            (*existing_ad_it)->text = entry->text;
        }

        if(entry->active != (*existing_ad_it)->active)
        {
            info_updated = true;
            (*existing_ad_it)->active = entry->active;
        }

        if(entry->expiring_on != (*existing_ad_it)->expiring_on)
        {
            info_updated = true;
            (*existing_ad_it)->expiring_on = entry->expiring_on;
        }

        if(!info_updated)
            return;
    }
    Logger::write(": INFO : BAS : ADS : [" + std::to_string(entry->id) + "] [" + entry->owner + "] UPDATED.");
}

template<>
void Database<UserExtended>::sync()
{
    try
    {
        copy_sql_file();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        last_err_msg_ = ex.what();
        Logger::write(": ERROR : FIL : " + last_err_msg_);

        throw ex;
    }

    std::string query;

    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);
        std::for_each(vec_.begin(), vec_.end(), [&](const dataPtr& user)
        {
            send_query(
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
                        + std::string(", tg_activetasks=") + std::to_string(user->activeTasks.to_ulong())
                        + std::string(" WHERE tg_id=") + std::to_string(user->id)
                    );
        });
    }

    Logger::write(": INFO : BAS : USR : SYNC OK.");
}

template<>
void Database<Ad>::sync()
{
    try
    {
        copy_sql_file();
    }
    catch(const boost::filesystem::filesystem_error& ex)
    {
        last_err_msg_ = ex.what();
        Logger::write(": ERROR : FIL : " + last_err_msg_);

        throw ex;
    }

    std::string query;

    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);
        std::for_each(vec_.begin(), vec_.end(), [&](const dataPtr& entry)
        {
            send_query(
                        (std::string)"UPDATE ads SET owner='" + std::string(entry->owner)
                        + std::string("', text='") + std::string(entry->text)
                        + std::string("', active=") + std::string(entry->active ? "TRUE" : "FALSE")
                        + std::string(", expiring_on=")+ std::to_string(entry->expiring_on)
                        + std::string(" WHERE id=") + std::to_string(entry->id)
                    );
        });
    }

    Logger::write(": INFO : BAS : ADS : SYNC OK.");
}

template<>
void Database<UserExtended>::show_table(std::ostream& os)
{
    os << std::left << std::setw(16) << "ID" << std::setw(32) << "USERNAME" << "FIRSTNAME" << std::endl;

    std::lock_guard<std::mutex> lock_vec(mutex_vec_);
    std::for_each(vec_.begin(), vec_.end(),[&os](const dataPtr& entry)
    {
        os << std::left << std::setw(16) << std::to_string(entry->id) << std::setw(32) << entry->username << entry->firstName << std::endl;
    });
}

template<>
void Database<Ad>::show_table(std::ostream& os)
{
    os << std::left << std::setw(16) << "ID" << std::setw(32) << "OWNER" << "EXPIRING ON" << std::endl;
    std::lock_guard<std::mutex> lock_vec(mutex_vec_);
    std::for_each(vec_.begin(), vec_.end(),[&os](const dataPtr& entry)
    {
        std::time_t now = entry->expiring_on;
        os << std::left << std::setw(16) << std::to_string(entry->id) << std::setw(32) << entry->owner << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << std::endl;
    });
}
