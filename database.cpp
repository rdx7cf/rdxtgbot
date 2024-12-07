#include "database.h"

///////////////////////
// AUX SECTION OPEN //
/////////////////////

/*static std::vector<std::string> split(const std::string& str, char delim) noexcept
{
    using iter = std::string::const_iterator;

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
}*/

static std::string string_shortener(const std::string& str, std::string::size_type desired_sz) noexcept
{
    auto sz = str.size();

    if(sz >= desired_sz)
        return std::string(str, 0, desired_sz - 3) + std::string("...");
    else if(str[sz - 1] == '\n')
        return std::string(str, 0, sz - 1);
    else
        return str;
}

std::vector<TmExtended> extract_schedule(const std::string& raw_tpoint, const std::string& raw_wday) noexcept
{

    std::vector<TmExtended> result;
    std::stringstream raw_tpoint_stream(raw_tpoint);

    for(std::string tpoint_str; std::getline(raw_tpoint_stream, tpoint_str, ' '); )
    {
        auto tpoint_splitted = StringTools::split(tpoint_str, ':');

        TmExtended t {};

        try
        {
            t.tm_hour = std::stoi(tpoint_splitted[0]);
            t.tm_min = std::stoi(tpoint_splitted[1]);
        }
        catch(...)
        {
            Logger::write(": ERROR : DATABASE : Invalid schedule timepoint formatting: '" + tpoint_str + "'.");
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
                Logger::write(": ERROR : DATABASE : Invalid schedule weekday formatting: '" + wday_str + "'.");
                continue;
            }

            result.push_back(t);
        }


    }

    return result;
}

static int extract_user(void* users, int colcount, char** columns, char** colnames)
{
    UserExtended::Ptr entry = std::make_shared<UserExtended>();

    entry->id = std::stol(columns[1]);
    entry->username = columns[2];
    entry->firstName = columns[3];
    entry->lastName = columns[4];
    entry->languageCode = columns[5];
    entry->isBot = std::stoi(columns[6]);
    entry->isPremium = std::stoi(columns[7]);
    entry->addedToAttachmentMenu = std::stoi(columns[8]);
    entry->canJoinGroups = std::stoi(columns[9]);
    entry->canReadAllGroupMessages = std::stoi(columns[10]);
    entry->supportsInlineQueries = std::stoi(columns[11]);
    entry->activeTasks = std::stoul(columns[12], 0, 2);
    entry->member_since = std::stol(columns[13]);

    static_cast<std::vector<UserExtended::Ptr>*>(users)->push_back(entry);

    return 0;
}

static int extract_notif(void* notifs, int colcount, char** columns, char** colnames)
{
    Notification::Ptr entry = std::make_shared<Notification>();

    entry->id = std::stol(columns[0]);
    entry->owner = columns[1];
    entry->text = columns[2];
    entry->active = std::stoi(columns[3]);
    entry->type = static_cast<Notification::TYPE>(std::stoi(columns[4]));
    entry->tpoints_str = columns[5];
    entry->wdays_str = columns[6];
    entry->schedule = extract_schedule(entry->tpoints_str, entry->wdays_str);
    if(entry->schedule.size() == 0)
    {
        Logger::write(": WARN : DATABASE : No schedule specified for: '" + entry->owner + "'.");
        entry->active = false;
        entry->tpoints_str.clear();
        entry->wdays_str.clear();
    }

    entry->added_on = std::stol(columns[7]);
    entry->expiring_on = std::stol(columns[8]);

    static_cast<std::vector<Notification::Ptr>*>(notifs)->push_back(entry);

    return 0;
}

static int extract_vps(void* vps, int colcount, char** columns, char** colnames)
{
    VPS::Ptr entry = std::make_shared<VPS>();

    entry->id = std::stol(columns[0]);
    entry->owner = std::stol(columns[1]);
    entry->uuid = columns[2];
    entry->name = columns[3];

    static_cast<std::vector<VPS::Ptr>*>(vps)->push_back(entry);

    return 0;
}

////////////////////////
// AUX SECTION CLOSE //
//////////////////////

// USERBASE

Userbase::Userbase(const Database<UserExtended>::sPtrF& file, int interval) : Database<UserExtended>(file, interval)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);
        file_->send_query
                (
                    "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, tg_id INTEGER UNIQUE, tg_uname TEXT, tg_fname TEXT, tg_lname TEXT, tg_langcode TEXT, tg_bot BOOLEAN, tg_prem BOOLEAN, tg_ATAM BOOLEAN, tg_CJG BOOLEAN, tg_CRAGM BOOLEAN, tg_SIQ BOOLEAN, activetasks TEXT, membersince INTEGER);"
                    "SELECT * FROM users",
                    extract_user,
                    &vec_
                );

    }

    Logger::write(": INFO : DATABASE : Userbase has been initialized.");
}

bool Userbase::add(const UserExtended::Ptr& entry)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);

        if(find_if([&entry](const UserExtended::Ptr& user) { return user->id == entry->id; }) != vec_.end())
            return false;

        vec_.push_back(entry);
    }

    file_->send_query(
        (std::string)"INSERT INTO users (tg_id, tg_uname, tg_fname, tg_lname, tg_langcode, tg_bot, tg_prem, tg_ATAM, tg_CJG, tg_CRAGM, tg_SIQ, activetasks, membersince) VALUES ("
        + std::to_string(entry->id)
        + std::string(", '")
        + entry->username
        + std::string("', '")
        + entry->firstName
        + std::string("', '")
        + entry->lastName
        + std::string("', '")
        + entry->languageCode
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
        + std::string(", '")
        + entry->activeTasks.to_string()
        + std::string("', ")
        + std::to_string(entry->member_since)
        + std::string(");"));


    Logger::write(": INFO : DATABASE : New user [" + std::to_string(entry->id) + "] [" + entry->firstName + "] has been added.");

    return true;
}

bool Userbase::update(const UserExtended::Ptr& entry) noexcept
{

    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mtx_vec_);

        // Searching for the user in the vector.

        auto existing_user_it = find_if([&entry](const UserExtended::Ptr& user) { return user->id == entry->id; });

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

        if(entry->activeTasks != (*existing_user_it)->activeTasks)
        {
            info_updated = true;
            (*existing_user_it)->activeTasks = entry->activeTasks;
        }

        if(!info_updated)
            return false;
    }
    Logger::write(": INFO : DATABASE : User [" + std::to_string(entry->id) + "] [" + entry->firstName + "] has been updated.");

    return true;
}

void Userbase::sync() const
{
    auto f = [this](const UserExtended::Ptr& user) // Для замыканий лучше использовать auto, а не std::function. Это не одно и то же: std::function для замыканий работает медленно и занимает больше памяти.
    {
        file_->send_query(
                    (std::string)"UPDATE users SET tg_uname='" + user->username
                    + std::string("', tg_fname='") + user->firstName
                    + std::string("', tg_lname='") + user->lastName
                    + std::string("', tg_langcode='")+ user->languageCode
                    + std::string("', tg_bot=") + std::to_string(user->isBot)
                    + std::string(", tg_prem=") + std::to_string(user->isPremium)
                    + std::string(", tg_ATAM=") + std::to_string(user->addedToAttachmentMenu)
                    + std::string(", tg_CJG=") + std::to_string(user->canJoinGroups)
                    + std::string(", tg_CRAGM=") + std::to_string(user->canReadAllGroupMessages)
                    + std::string(", tg_SIQ=") + std::to_string(user->supportsInlineQueries)
                    + std::string(", activetasks='") + user->activeTasks.to_string()
                    + std::string("', membersince=") + std::to_string(user->member_since)
                    + std::string(" WHERE tg_id=") + std::to_string(user->id)
                );
    };
    for_range(f);

    Logger::write(": INFO : DATABASE : Users have been synced with the SQL file.");
}

void Userbase::show_table(std::ostream& os) const noexcept
{
    os << std::endl << std::left
       << std::setw(18) << "ID"
       << std::setw(8) << "TASKS"
       << std::setw(6) << "BOT"
       << std::setw(6) << "LANG"
       << std::setw(6) << "PREM"
       << std::setw(18) << "USERNAME"
       << std::setw(18) << "FIRSTNAME" << "\t"
       << "MEMBER SINCE"
       << std::endl;

    auto f = [&os](const UserExtended::Ptr& entry)
    {
        std::tm ms = localtime_ts(entry->member_since);

        os << std::left
           << std::setw(18) << std::to_string(entry->id)
           << std::setw(8) <<  entry->activeTasks.to_string()
           << std::setw(6) <<  (entry->isBot ? "Yes" : "No")
           << std::setw(6) <<  entry->languageCode
           << std::setw(6) <<  (entry->isPremium ? "Yes" : "No")
           << std::setw(18) << string_shortener(entry->username, 16)
           << std::setw(18) << string_shortener(entry->firstName, 16) << "\t"
           << std::put_time(&ms, "%d-%m-%Y %H:%M:%S")
           << std::endl;
    };
    for_range(f);
}


// NOTIFBASE

Notifbase::Notifbase(const Database<Notification>::sPtrF& file, int interval) : Database<Notification>(file, interval)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);
        file_->send_query
                (
                    "CREATE TABLE IF NOT EXISTS notifications (id INTEGER PRIMARY KEY AUTOINCREMENT,owner TEXT,text TEXT,active BOOLEAN, type INTEGER,tpoints TEXT,wdays TEXT,added_on INTEGER,expiring_on INTEGER);"
                    "SELECT * FROM notifications",
                    extract_notif,
                    &vec_
                );
    }

    Logger::write(": INFO : DATABASE : Notifbase has been initialized.");
}

bool Notifbase::add(const Notification::Ptr& entry)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);

        if(find_if([&entry](const Notification::Ptr& notif) { return notif->id == entry->id; }) != vec_.end())
            return false;

        vec_.push_back(entry);
    }

    file_->send_query(
        (std::string)"INSERT INTO notifications (owner, text, active, type, tpoints, wdays, added_on, expiring_on) VALUES ('"
        + std::string(entry->owner)
        + std::string("', '")
        + std::string(entry->text)
        + std::string("', ")
        + std::to_string(entry->active)
        + std::string(", '")
        + std::to_string(static_cast<int>(entry->type))
        + std::string("', '")
        + entry->tpoints_str
        + std::string("', '")
        + entry->wdays_str
        + std::string("', ")
        + std::to_string(entry->added_on)
        + std::string(", ")
        + std::to_string(entry->expiring_on)
        + std::string(");"));

    Logger::write(": INFO : DATABASE : New notification [" + std::to_string(entry->id) + "] [" + entry->owner + "] has been added.");

    return true;
}

bool Notifbase::update(const Notification::Ptr& entry) noexcept
{
    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mtx_vec_);

        // Searching for the ad in the vector.

        auto existing_notif_it = find_if([&entry](const Notification::Ptr& notif) { return notif->id == entry->id; });

        if(existing_notif_it == vec_.end())
            return false;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(entry->owner != (*existing_notif_it)->owner)
        {
            info_updated = true;
            (*existing_notif_it)->owner = entry->owner;
        }

        if(entry->text != (*existing_notif_it)->text)
        {
            info_updated = true;
            (*existing_notif_it)->text = entry->text;
        }

        if(entry->active != (*existing_notif_it)->active)
        {
            info_updated = true;
            (*existing_notif_it)->active = entry->active;
        }

        if(entry->type != (*existing_notif_it)->type)
        {
            info_updated = true;
            (*existing_notif_it)->type = entry->type;
        }

        if(entry->tpoints_str != (*existing_notif_it)->tpoints_str)
        {
            info_updated = true;
            (*existing_notif_it)->schedule = entry->schedule;
            (*existing_notif_it)->tpoints_str = entry->tpoints_str;
            (*existing_notif_it)->wdays_str = entry->wdays_str;
        }

        if(entry->wdays_str != (*existing_notif_it)->wdays_str)
        {
            info_updated = true;
            (*existing_notif_it)->schedule = entry->schedule;
            (*existing_notif_it)->wdays_str = entry->wdays_str;
        }

        if(entry->added_on != (*existing_notif_it)->added_on)
        {
            info_updated = true;
            (*existing_notif_it)->added_on = entry->added_on;
        }

        if(entry->expiring_on != (*existing_notif_it)->expiring_on)
        {
            info_updated = true;
            (*existing_notif_it)->expiring_on = entry->expiring_on;
        }

        if(!info_updated)
            return false;
    }

    file_->send_query(
                (std::string)"UPDATE notifications SET owner='" + std::string(entry->owner)
                + std::string("', text='") + std::string(entry->text)
                + std::string("', active=") + std::to_string(entry->active)
                + std::string(", type=") + std::to_string(static_cast<int>(entry->type))
                + std::string(", tpoints='") + entry->tpoints_str
                + std::string("', wdays='") + entry->wdays_str
                + std::string("', added_on=") + std::to_string(entry->added_on)
                + std::string(", expiring_on=") + std::to_string(entry->expiring_on)
                + std::string(" WHERE id=") + std::to_string(entry->id)
            );

    Logger::write(": INFO : DATABASE : Notification [" + std::to_string(entry->id) + "] [" + entry->owner + "] has been updated.");

    return true;
}

void Notifbase::sync() const
{
    auto f = [this](const Notification::Ptr& entry)
    {
        file_->send_query(
                    (std::string)"UPDATE notifications SET owner='" + std::string(entry->owner)
                    + std::string("', text='") + std::string(entry->text)
                    + std::string("', active=") + std::to_string(entry->active)
                    + std::string(", type=") + std::to_string(static_cast<int>(entry->type))
                    + std::string(", tpoints='") + entry->tpoints_str
                    + std::string("', wdays='") + entry->wdays_str
                    + std::string("', added_on=") + std::to_string(entry->added_on)
                    + std::string(", expiring_on=") + std::to_string(entry->expiring_on)
                    + std::string(" WHERE id=") + std::to_string(entry->id)
                );
    };
    for_range(f);

    Logger::write(": INFO : DATABASE : Notifications have been synced with the SQL base.");
}

void Notifbase::show_table(std::ostream& os) const noexcept
{
    os << std::endl
       << std::left << std::setw(6) << "ID"
       << std::setw(8) << "ACTIVE"
       << std::setw(8) << "TYPE"
       << std::setw(18) << "OWNER"
       << std::setw(18) << "TEXT"
       << std::setw(18) << "SCHEDULE"
       << "ADDED ON"
       << "\t\t\t" << "EXPIRING ON"
       << std::endl;

    auto f = [&os](const Notification::Ptr& entry)
    {
        std::tm added_on = localtime_ts(entry->added_on);
        std::tm expiring_on = localtime_ts(entry->expiring_on);
        os << std::left
           << std::setw(6) << std::to_string(entry->id)
           << std::setw(8) << (entry->active ? "Yes" : "No")
           << std::setw(8) << std::to_string(static_cast<int>(entry->type))
           << std::setw(18) << string_shortener(entry->owner, 16)
           << std::setw(18) << string_shortener(entry->text, 16)
           << std::setw(18) << string_shortener(entry->tpoints_str, 16)
           << std::put_time(&added_on, "%d-%m-%Y %H:%M:%S")
           << "\t\t" << std::put_time(&expiring_on, "%d-%m-%Y %H:%M:%S")
           << std::endl;
    };

    for_range(f);
}

// VPSBASE


VPSbase::VPSbase(const Database<VPS>::sPtrF& file, int interval) : Database<VPS>(file, interval)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);
        file_->send_query
                (
                    "CREATE TABLE IF NOT EXISTS vps (id INTEGER PRIMARY KEY AUTOINCREMENT,owner INTEGER,uuid TEXT,name TEXT);"
                    "SELECT * FROM vps",
                    extract_vps,
                    &vec_
                );
    }

    Logger::write(": INFO : DATABASE : VPSbase has been initialized.");
}

bool VPSbase::add(const VPS::Ptr& entry)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);

        if(find_if([&entry](const VPS::Ptr& vps) { return vps->id == entry->id; }) != vec_.end())
            return false;

        vec_.push_back(entry);
    }

    file_->send_query(
        (std::string)"INSERT INTO vps (owner, uuid, name) VALUES ("
        + std::to_string(entry->owner)
        + std::string(", '")
        + entry->uuid
        + std::string("', '")
        + entry->name
        + std::string("');"));

    Logger::write(": INFO : DATABASE : New VPS [" + std::to_string(entry->id) + "] [" + std::to_string(entry->owner) + "] has been added.");

    return true;
}

bool VPSbase::update(const VPS::Ptr& entry) noexcept
{
    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mtx_vec_);

        // Searching for the ad in the vector.

        auto existing_vps_it = find_if([&entry](const VPS::Ptr& vps) { return vps->id == entry->id; });

        if(existing_vps_it == vec_.end())
            return false;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(entry->owner != (*existing_vps_it)->owner)
        {
            info_updated = true;
            (*existing_vps_it)->owner = entry->owner;
        }

        if(entry->uuid != (*existing_vps_it)->uuid)
        {
            info_updated = true;
            (*existing_vps_it)->uuid = entry->uuid;
        }

        if(entry->name != (*existing_vps_it)->name)
        {
            info_updated = true;
            (*existing_vps_it)->name = entry->name;
        }

        if(!info_updated)
            return false;
    }

    file_->send_query(
                (std::string)"UPDATE vps SET owner=" + std::to_string(entry->owner)
                + std::string(", uuid='") + entry->uuid
                + std::string("', name='") + entry->name
                + std::string("' WHERE id=") + std::to_string(entry->id)
            );

    Logger::write(": INFO : DATABASE : A VPS [" + std::to_string(entry->id) + "] [" + std::to_string(entry->owner) + "] has been updated.");

    return true;
}

void VPSbase::sync() const
{
    auto f = [this](const VPS::Ptr& entry)
    {
        file_->send_query(
                    (std::string)"UPDATE vps SET owner=" + std::to_string(entry->owner)
                    + std::string(", uuid='") + entry->uuid
                    + std::string("', name='") + entry->name
                    + std::string("' WHERE id=") + std::to_string(entry->id)
                );
    };
    for_range(f);

    Logger::write(": INFO : DATABASE : VPS base have been synced with the SQL base.");
}

void VPSbase::show_table(std::ostream& os) const noexcept
{
    os << std::endl
       << std::left << std::setw(6) << "ID"
       << std::setw(18) << "OWNER"
       << std::setw(18) << "UUID"
       << std::setw(18) << "NAME"
       << std::endl;

    auto f = [&os](const VPS::Ptr& entry)
    {
        os << std::left
           << std::setw(6) << std::to_string(entry->id)
           << std::setw(18) << string_shortener(std::to_string(entry->owner), 16)
           << std::setw(18) << string_shortener(entry->uuid, 16)
           << std::setw(18) << string_shortener(entry->name, 16)
           << std::endl;
    };

    for_range(f);
}

VPS::Ptr VPSbase::get_copy_by_owner_n_name(std::int64_t owner, const std::string& name)
{
    std::lock_guard<std::mutex> lock_vec(mtx_vec_);

    auto current_it = find_if([&owner, &name](const VPS::Ptr& entry) {
        return entry->owner == owner && entry->name == name;
    });

    if(current_it == vec_.cend())
        return nullptr;

    return std::make_shared<VPS>(*(*current_it));
}

