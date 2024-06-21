#include "database.h"

///////////////////////
// AUX SECTION OPEN //
/////////////////////

static std::vector<std::string> split(const std::string& str, char delim)
{
    typedef std::string::const_iterator iter;

    std::vector<std::string> ret;

    iter i = str.begin();
    while(i != str.end())
    {
        i = find_if(i, str.end(), [&delim](const char c){return c != delim;});
        iter j = find_if(i, str.end(), [&delim](const char c){return c == delim;});

        if(i != str.end())
            ret.push_back(std::string(i, j));

        i = j;
    }

    return ret;
}

static std::string string_shortener(const std::string& str, std::string::size_type desired_sz)
{
    auto sz = str.size();

    if(sz >= desired_sz)
        return std::string(str, 0, desired_sz - 3) + std::string("...");
    else if(str[sz - 1] == '\n')
        return std::string(str, 0, sz - 1);
    else
        return str;
}

std::vector<TmExtended> extract_schedule(const std::string& raw_tpoint, const std::string& raw_wday)
{

    std::vector<TmExtended> result;
    std::stringstream raw_tpoint_stream(raw_tpoint);

    for(std::string tpoint_str; std::getline(raw_tpoint_stream, tpoint_str, ' '); )
    {
        auto tpoint_splitted = split(tpoint_str, ':');

        TmExtended t {};

        try
        {
            t.tm_hour = std::stoi(tpoint_splitted[0]);
            t.tm_min = std::stoi(tpoint_splitted[1]);
        }
        catch(...)
        {
            Logger::write(": ERROR : BAS : Invalid schedule timepoint formatting: '" + tpoint_str + "'.");
            continue;
        }

        std::stringstream raw_wday_stream(raw_wday);
        for(std::string wday_str; std::getline(raw_wday_stream, wday_str, ' '); )
        {
            try
            {
                int wday = std::stoi(wday_str);

                if(wday < 0 || 6 < wday)
                    throw std::exception();

                t.tm_wday = std::stoi(wday_str);

            }
            catch(...)
            {
                Logger::write(": ERROR : BAS : Invalid schedule weekday formatting: '" + wday_str + "'.");
                continue;
            }

            result.push_back(t);
        }


    }

    return result;
}

static int extract_user(void* users, int colcount, char** columns, char** colnames)
{
    UserExtended::Ptr user(new UserExtended);

    user->id = std::stol(columns[1]);
    user->username = columns[2];
    user->firstName = columns[3];
    user->lastName = columns[4];
    user->languageCode = columns[5];
    user->isBot = std::stoi(columns[6]);
    user->isPremium = std::stoi(columns[7]);
    user->addedToAttachmentMenu = std::stoi(columns[8]);
    user->canJoinGroups = std::stoi(columns[9]);
    user->canReadAllGroupMessages = std::stoi(columns[10]);
    user->supportsInlineQueries = std::stoi(columns[11]);
    user->activeTasks = std::stoul(columns[12]);
    user->blocked = std::stoi(columns[13]);
    user->member_since = std::stol(columns[14]);

    static_cast<std::vector<UserExtended::Ptr>*>(users)->push_back(user);

    return 0;
}

static int extract_ad(void* ads, int colcount, char** columns, char** colnames)
{
    Ad::Ptr ad(new Ad);

    ad->id = std::stol(columns[0]);
    ad->owner = columns[1];
    ad->text = columns[2];
    ad->active = std::stoi(columns[3]);

    ad->tpoints_str = columns[4];
    ad->wdays_str = columns[5];
    ad->schedule = extract_schedule(ad->tpoints_str, ad->wdays_str);
    if(ad->schedule.size() == 0)
    {
        Logger::write(": WARN : BAS : No schedule specified for: '" + ad->owner + "'.");
        ad->active = false;
        ad->tpoints_str.clear();
        ad->wdays_str.clear();
    }

    ad->added_on = std::stol(columns[6]);
    ad->expiring_on = std::stol(columns[7]);

    static_cast<std::vector<Ad::Ptr>*>(ads)->push_back(ad);

    return 0;
}

////////////////////////
// AUX SECTION CLOSE //
//////////////////////

// DATABASE

std::mutex Database::mutex_sql_ {}; // ODR-use.

void Database::send_query(const std::string& query, int (*callback)(void*, int, char**, char**), void* container)
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

        Logger::write(": ERROR : BAS : " + last_err_msg_ + " :: QUERY :: " + query);

        sqlite3_free(err_msg);
        sqlite3_close(db);
    }

    sqlite3_close(db);
}

void Database::copy_sql_file()
{
    std::lock_guard<std::mutex> lock(mutex_sql_); // Declaring a lock_guard with the same SQL mutex before calling this function leads to deadlock.
    boost::filesystem::copy_file(filename_, filename_ + ".bak", boost::filesystem::copy_options::overwrite_existing);
    Logger::write(": INFO : FIL : '" + filename_ + "' COPIED.");
}

// USERBASE

Userbase::Userbase(const std::string& filename) : Database(filename)
{
    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        send_query
                (
                    "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, tg_id INTEGER UNIQUE, tg_uname TEXT, tg_fname TEXT, tg_lname TEXT, tg_langcode TEXT, tg_bot BOOLEAN, tg_prem BOOLEAN, tg_ATAM BOOLEAN, tg_CJG BOOLEAN, tg_CRAGM BOOLEAN, tg_SIQ BOOLEAN, tg_activetasks INTEGER, tg_blocked BOOLEAN, tg_membersince INTEGER);"
                    "SELECT * FROM users",
                    extract_user,
                    &vec_
                );

    }

    Logger::write(": INFO : BAS : USR : INITIALIZED.");
}

bool Userbase::add(const UserExtended::Ptr& entry)
{
    {
        std::lock_guard<std::mutex> lock(mutex_vec_);

        if(get_by_id(entry->id) != vec_.end())
            return false;

        vec_.push_back(entry);
    }

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
        (std::string)"INSERT INTO users (tg_id, tg_uname, tg_fname, tg_lname, tg_langcode, tg_bot, tg_prem, tg_ATAM, tg_CJG, tg_CRAGM, tg_SIQ, tg_activetasks, tg_blocked, tg_membersince) VALUES ("
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
        + std::to_string(entry->isBot)
        + std::string(", ")
        + std::to_string(entry->isPremium)
        + std::string(", ")
        + std::to_string(entry->addedToAttachmentMenu)
        + std::string(", ")
        + std::to_string(entry->canJoinGroups)
        + std::string(", ")
        + std::to_string(entry->canReadAllGroupMessages)
        + std::string(", ")
        + std::to_string(entry->supportsInlineQueries)
        + std::string(", ")
        + std::to_string(entry->activeTasks.to_ulong())
        + std::string(", ")
        + std::to_string(entry->blocked)
        + std::string(", ")
        + std::to_string(entry->member_since)
        + std::string(");"));


    Logger::write(": INFO : BAS : USR : [" + std::to_string(entry->id) + "] [" + entry->firstName + "] ADDED.");

    return true;
}

bool Userbase::update(const UserExtended::Ptr& entry)
{

    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);

        // Searching for the user in the vector.

        auto existing_user_it = get_by_id(entry->id);

        if(existing_user_it == vec_.end())
            return false;

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

        if(entry->blocked != (*existing_user_it)->blocked)
        {
            info_updated = true;
            (*existing_user_it)->blocked = entry->blocked;
        }

        if(!info_updated)
            return false;
    }
    Logger::write(": INFO : BAS : USR : [" + std::to_string(entry->id) + "] [" + entry->firstName + "] UPDATED.");

    return true;
}

Userbase::iterator Userbase::get_by_id(std::int64_t id)
{
    return find_if(vec_.begin(), vec_.end(), [&id](const UserExtended::Ptr& x) { return x->id == id; });
}

void Userbase::sync()
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

    auto f = [this](UserExtended::Ptr& user) // Для замыканий лучше использовать auto, а не std::function. Это не одно и то же: std::function для замыканий работает медленно и занимает больше памяти.
    {
        send_query(
                    (std::string)"UPDATE users SET tg_uname='" + std::string(user->username)
                    + std::string("', tg_fname='") + std::string(user->firstName)
                    + std::string("', tg_lname='") + std::string(user->lastName)
                    + std::string("', tg_langcode='")+ std::string(user->languageCode)
                    + std::string("', tg_bot=") + std::to_string(user->isBot)
                    + std::string(", tg_prem=") + std::to_string(user->isPremium)
                    + std::string(", tg_ATAM=") + std::to_string(user->addedToAttachmentMenu)
                    + std::string(", tg_CJG=") + std::to_string(user->canJoinGroups)
                    + std::string(", tg_CRAGM=") + std::to_string(user->canReadAllGroupMessages)
                    + std::string(", tg_SIQ=") + std::to_string(user->supportsInlineQueries)
                    + std::string(", tg_activetasks=") + std::to_string(user->activeTasks.to_ulong())
                    + std::string(", tg_blocked=") + std::to_string(user->blocked)
                    + std::string(", tg_membersince=") + std::to_string(user->member_since)
                    + std::string(" WHERE tg_id=") + std::to_string(user->id)
                );
    };
    for_range(f);

    Logger::write(": INFO : BAS : USR : SYNC OK.");
}

void Userbase::show_table(std::ostream& os)
{
    os << std::endl << std::left
       << std::setw(18) << "ID"
       << std::setw(8) << "TASKS"
       << std::setw(6) << "BOT"
       << std::setw(6) << "LANG"
       << std::setw(6) << "PREM"
       << std::setw(7) << "BLOCK"
       << std::setw(18) << "USERNAME"
       << std::setw(18) << "FIRSTNAME"
       << "MEMBER SINCE"
       << std::endl;

    auto f = [&os](UserExtended::Ptr& entry)
    {
        std::tm ms = localtime_ts(entry->member_since);

        os << std::left
           << std::setw(18) << std::to_string(entry->id)
           << std::setw(8) <<  entry->activeTasks.to_string()
           << std::setw(6) <<  (entry->isBot ? "Yes" : "No")
           << std::setw(6) <<  entry->languageCode
           << std::setw(6) <<  (entry->isPremium ? "Yes" : "No")
           << std::setw(7) <<  (entry->blocked ? "Yes" : "No")
           << std::setw(18) << string_shortener(entry->username, 16)
           << std::setw(18) << string_shortener(entry->firstName, 16)
           << std::put_time(&ms, "%d-%m-%Y %H:%M:%S")
           << std::endl;
    };
    for_range(f);
}

void Userbase::for_range(const std::function<void(UserExtended::Ptr&)>& f)
{
    std::lock_guard<std::mutex> lock_vec(mutex_vec_);
    std::for_each(vec_.begin(), vec_.end(), f);
}

UserExtended::Ptr Userbase::get_copy_by_id(int64_t id)
{
    auto current_it = get_by_id(id);
    if(current_it == vec_.end())
        return nullptr;
    return UserExtended::Ptr(new UserExtended(*(*current_it)));
}

// ADBASE

Adbase::Adbase(const std::string& filename) : Database(filename)
{
    {
        std::lock_guard<std::mutex> lock(mutex_vec_);
        send_query
                (
                    "CREATE TABLE IF NOT EXISTS ads (id INTEGER PRIMARY KEY AUTOINCREMENT,owner TEXT,text TEXT,active BOOLEAN,tpoints TEXT,wdays TEXT,added_on INTEGER,expiring_on INTEGER);"
                    "SELECT * FROM ads",
                    extract_ad,
                    &vec_
                );
    }

    Logger::write(": INFO : BAS : USR : INITIALIZED.");
}

bool Adbase::add(const Ad::Ptr& entry)
{
    {
        std::lock_guard<std::mutex> lock(mutex_vec_);

        if(get_by_id(entry->id) != vec_.end())
            return false;

        vec_.push_back(entry);
    }

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
        (std::string)"INSERT INTO ads (owner, text, active, tpoints, wdays, added_on, expiring_on) VALUES ('"
        + std::string(entry->owner)
        + std::string("', '")
        + std::string(entry->text)
        + std::string("', ")
        + std::to_string(entry->active)
        + std::string(", '")
        + entry->tpoints_str
        + std::string("', '")
        + entry->wdays_str
        + std::string("', ")
        + std::to_string(entry->added_on)
        + std::string(", ")
        + std::to_string(entry->expiring_on)
        + std::string(");"));

    Logger::write(": INFO : BAS : ADS : [" + std::to_string(entry->id) + "] [" + entry->owner + "] ADDED.");

    return true;
}

bool Adbase::update(const Ad::Ptr& entry)
{
    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mutex_vec_);

        // Searching for the ad in the vector.

        auto existing_ad_it = get_by_id(entry->id);

        if(existing_ad_it == vec_.end())
            return false;

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

        if(entry->tpoints_str != (*existing_ad_it)->tpoints_str)
        {
            info_updated = true;
            (*existing_ad_it)->schedule = entry->schedule;
            (*existing_ad_it)->tpoints_str = entry->tpoints_str;
            (*existing_ad_it)->wdays_str = entry->wdays_str;
        }

        if(entry->wdays_str != (*existing_ad_it)->wdays_str)
        {
            info_updated = true;
            (*existing_ad_it)->schedule = entry->schedule;
            (*existing_ad_it)->wdays_str = entry->wdays_str;
        }

        if(entry->added_on != (*existing_ad_it)->added_on)
        {
            info_updated = true;
            (*existing_ad_it)->added_on = entry->added_on;
        }

        if(entry->expiring_on != (*existing_ad_it)->expiring_on)
        {
            info_updated = true;
            (*existing_ad_it)->expiring_on = entry->expiring_on;
        }

        if(!info_updated)
            return false;
    }
    Logger::write(": INFO : BAS : ADS : [" + std::to_string(entry->id) + "] [" + entry->owner + "] UPDATED.");

    send_query(
                (std::string)"UPDATE ads SET owner='" + std::string(entry->owner)
                + std::string("', text='") + std::string(entry->text)
                + std::string("', active=") + std::to_string(entry->active)
                + std::string(", tpoints='") + entry->tpoints_str
                + std::string("', wdays='") + entry->wdays_str
                + std::string("', added_on=") + std::to_string(entry->added_on)
                + std::string(", expiring_on=") + std::to_string(entry->expiring_on)
                + std::string(" WHERE id=") + std::to_string(entry->id)
            );

    return true;
}

Adbase::iterator Adbase::get_by_id(std::int64_t id)
{
    return find_if(vec_.begin(), vec_.end(), [&id](const Ad::Ptr& x) { return x->id == id; });
}

void Adbase::sync()
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

    auto f = [this](Ad::Ptr& entry)
    {
        send_query(
                    (std::string)"UPDATE ads SET owner='" + std::string(entry->owner)
                    + std::string("', text='") + std::string(entry->text)
                    + std::string("', active=") + std::to_string(entry->active)
                    + std::string(", tpoints='") + entry->tpoints_str
                    + std::string("', wdays='") + entry->wdays_str
                    + std::string("', added_on=") + std::to_string(entry->added_on)
                    + std::string(", expiring_on=") + std::to_string(entry->expiring_on)
                    + std::string(" WHERE id=") + std::to_string(entry->id)
                );
    };
    for_range(f);

    Logger::write(": INFO : BAS : ADS : SYNC OK.");
}

void Adbase::show_table(std::ostream& os)
{
    os << std::endl
       << std::left << std::setw(6) << "ID"
       << std::setw(8) << "ACTIVE"
       << std::setw(18) << "OWNER"
       << std::setw(18) << "TEXT"
       << std::setw(18) << "SCHEDULE"
       << "ADDED ON"
       << "\t\t\t" << "EXPIRING ON"
       << std::endl;

    auto f = [&os](Ad::Ptr& entry)
    {
        std::tm added_on = localtime_ts(entry->added_on);
        std::tm expiring_on = localtime_ts(entry->expiring_on);
        os << std::left
           << std::setw(6) << std::to_string(entry->id)
           << std::setw(8) << (entry->active ? "Yes" : "No")
           << std::setw(18) << string_shortener(entry->owner, 16)
           << std::setw(18) << string_shortener(entry->text, 16)
           << std::setw(18) << string_shortener(entry->tpoints_str, 16)
           << std::put_time(&added_on, "%d-%m-%Y %H:%M:%S")
           << "\t\t" << std::put_time(&expiring_on, "%d-%m-%Y %H:%M:%S")
           << std::endl;
    };

    for_range(f);
}

void Adbase::for_range(const std::function<void(Ad::Ptr&)>& f)
{
    std::lock_guard<std::mutex> lock_vec(mutex_vec_);
    std::for_each(vec_.begin(), vec_.end(), f);
}

Ad::Ptr Adbase::get_copy_by_id(int64_t id)
{
    auto current_it = get_by_id(id);
    if(current_it == vec_.end())
        return nullptr;
    return Ad::Ptr(new Ad(*(*current_it)));
}
