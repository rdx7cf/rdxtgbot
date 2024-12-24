#include "Table.h"
#include "UserExtended.h"
#include "Notification.h"
#include "VPS.h"
#include "Auxiliary.h"

///////////////////////
// AUX SECTION OPEN //
/////////////////////

std::vector<TmExtended> extractSchedule(const std::string& raw_tpoint, const std::string& raw_wday) noexcept
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
    UserExtended::Ptr entry = std::make_shared<UserExtended>(
                std::stol(columns[1]),
                columns[2],
                columns[3],
                columns[4],
                columns[5],
                std::stoi(columns[6]),
                std::stoi(columns[7]),
                std::stoi(columns[8]),
                std::stoi(columns[9]),
                std::stoi(columns[10]),
                std::stoi(columns[11]),
                std::stoul(columns[12], 0, 2),
                std::stol(columns[13])
                );

    static_cast<std::vector<UserExtended::Ptr>*>(users)->push_back(entry);

    return 0;
}

static int extract_notif(void* notifs, int colcount, char** columns, char** colnames)
{
    Notification::Ptr entry = std::make_shared<Notification>(
                std::stol(columns[0]),
                columns[1],
                columns[2],
                std::stoi(columns[3]),
                static_cast<Notification::TYPE>(std::stoi(columns[4])),
                columns[5],
                columns[6],
                extractSchedule(columns[5], columns[6]),
                std::stol(columns[7]),
                std::stol(columns[8]),
                columns[9]);

    if(entry->schedule_.size() == 0)
    {
        Logger::write(": WARN : DATABASE : No schedule specified for: '" + entry->owner_ + "'.");
        entry->active_ = false;
        entry->tpoints_str_.clear();
        entry->wdays_str_.clear();
    }

    static_cast<std::vector<Notification::Ptr>*>(notifs)->push_back(entry);

    return 0;
}

static int extract_vps(void* vps, int colcount, char** columns, char** colnames)
{
    VPS::Ptr entry = std::make_shared<VPS>(
                columns[1],
                std::stol(columns[0]),
                std::stol(columns[2]),
                columns[3],
                columns[4],
                columns[5],
                columns[6]);

    static_cast<std::vector<VPS::Ptr>*>(vps)->push_back(entry);

    return 0;
}

////////////////////////
// AUX SECTION CLOSE //
//////////////////////

// USERBASE

UserTable::UserTable(const Table<UserExtended>::SptrF& file, int interval) : Table<UserExtended>(file, interval)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);
        file_->sendQuery
                (
                    "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, tg_id INTEGER UNIQUE, tg_uname TEXT, tg_fname TEXT, tg_lname TEXT, tg_langcode TEXT, tg_bot BOOLEAN, tg_prem BOOLEAN, tg_ATAM BOOLEAN, tg_CJG BOOLEAN, tg_CRAGM BOOLEAN, tg_SIQ BOOLEAN, activetasks TEXT, membersince INTEGER);"
                    "SELECT * FROM users",
                    extract_user,
                    &vec_
                );

    }

    Logger::write(": INFO : DATABASE : UserTable has been initialized.");
}

bool UserTable::add(const UserExtended::Ptr& entry)
{
    if(!Table<UserExtended>::add(entry))
        return false;

    file_->sendQuery(
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
        + entry->active_tasks_.to_string()
        + std::string("', ")
        + std::to_string(entry->member_since_)
        + std::string(");"));


    Logger::write(": INFO : DATABASE : New user [" + std::to_string(entry->id) + "] [" + entry->firstName + "] has been added.");

    return true;
}

bool UserTable::update(const UserExtended::Ptr& entry) noexcept
{
    // VECTOR MUTEX SCOPE LOCK
     {
         std::lock_guard<std::mutex> lock_vec(mtx_vec_);

         // Searching for the user in the vector.

         auto existing = getBy([&entry](const UserExtended::Ptr& user) { return user->id == entry->id; });

         if(!existing)
             return false;

         bool info_updated = false;

         // Updating the entry in the vector.

         if(entry->username != existing->username)
             existing->username = entry->username;


         if(entry->firstName != existing->firstName)
             existing->firstName = entry->firstName;

         if(entry->lastName != existing->lastName)
             existing->lastName = entry->lastName;

         if(entry->languageCode != existing->languageCode)
             existing->languageCode = entry->languageCode;

         if(entry->isBot != existing->isBot)
             existing->isBot = entry->isBot;

         if(entry->isPremium != existing->isPremium)
             existing->isPremium = entry->isPremium;

         if(entry->addedToAttachmentMenu != existing->addedToAttachmentMenu)
             existing->addedToAttachmentMenu = entry->addedToAttachmentMenu;

         if(entry->canJoinGroups != existing->canJoinGroups)
             existing->canJoinGroups = entry->canJoinGroups;

         if(entry->canReadAllGroupMessages != existing->canReadAllGroupMessages)
             existing->canReadAllGroupMessages = entry->canReadAllGroupMessages;

         if(entry->supportsInlineQueries != existing->supportsInlineQueries)
             existing->supportsInlineQueries = entry->supportsInlineQueries;

         if(entry->active_tasks_ != existing->active_tasks_)
         {
             info_updated = true;
             existing->active_tasks_ = entry->active_tasks_;
         }

         if(!info_updated)
             return false;
     }



    file_->sendQuery(
                std::string("UPDATE users SET activetasks='") + entry->active_tasks_.to_string()
                + std::string(" WHERE tg_id=") + std::to_string(entry->id)
            );

    Logger::write(": INFO : DATABASE : User [" + std::to_string(entry->id) + "] [" + entry->firstName + "] has been updated.");

    return true;
}

void UserTable::sync() const
{
    auto f = [this](const UserExtended::Ptr& user) // Для замыканий лучше использовать auto, а не std::function. Это не одно и то же: std::function для замыканий работает медленно и занимает больше памяти.
    {
        file_->sendQuery(
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
                    + std::string(", activetasks='") + user->active_tasks_.to_string()
                    + std::string("', membersince=") + std::to_string(user->member_since_)
                    + std::string(" WHERE tg_id=") + std::to_string(user->id)
                );
    };
    forRange(f);

    Logger::write(": INFO : DATABASE : Users have been synced with the SQL file.");
}

void UserTable::showTable(std::ostream& os) const noexcept
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
        std::tm ms = localtimeTs(entry->member_since_);

        os << std::left
           << std::setw(18) << std::to_string(entry->id)
           << std::setw(8) <<  entry->active_tasks_.to_string()
           << std::setw(6) <<  (entry->isBot ? "Yes" : "No")
           << std::setw(6) <<  entry->languageCode
           << std::setw(6) <<  (entry->isPremium ? "Yes" : "No")
           << std::setw(18) << AUX::shortenString(entry->username, 16)
           << std::setw(18) << AUX::shortenString(entry->firstName, 16) << "\t"
           << std::put_time(&ms, "%d-%m-%Y %H:%M:%S")
           << std::endl;
    };
    forRange(f);
}


// NOTIFBASE

NotificationTable::NotificationTable(const Table<Notification>::SptrF& file, int interval) : Table<Notification>(file, interval)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);
        file_->sendQuery
                (
                    "CREATE TABLE IF NOT EXISTS notifications (id INTEGER PRIMARY KEY AUTOINCREMENT,owner TEXT,text TEXT,active BOOLEAN, type INTEGER,tpoints TEXT,wdays TEXT,added_on INTEGER,expiring_on INTEGER,parse_mode TEXT);"
                    "SELECT * FROM notifications",
                    extract_notif,
                    &vec_
                );
    }

    Logger::write(": INFO : DATABASE : NotificationTable has been initialized.");
}

bool NotificationTable::add(const Notification::Ptr& entry)
{
    if(!Table<Notification>::add(entry))
        return false;

    file_->sendQuery(
        (std::string)"INSERT INTO notifications (owner, text, active, type, tpoints, wdays, added_on, expiring_on, parse_mode) VALUES ('"
        + std::string(entry->owner_)
        + std::string("', '")
        + std::string(entry->text_)
        + std::string("', ")
        + std::to_string(entry->active_)
        + std::string(", '")
        + std::to_string(static_cast<int>(entry->type_))
        + std::string("', '")
        + entry->tpoints_str_
        + std::string("', '")
        + entry->wdays_str_
        + std::string("', ")
        + std::to_string(entry->added_on_)
        + std::string(", ")
        + std::to_string(entry->expiring_on_)
        + std::string(", '")
        + entry->parse_mode_
        + std::string("');"));

    Logger::write(": INFO : DATABASE : New notification [" + std::to_string(entry->id_) + "] [" + entry->owner_ + "] has been added.");

    return true;
}

bool NotificationTable::update(const Notification::Ptr& entry) noexcept
{
    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mtx_vec_);

        // Searching for the ad in the vector.

        auto existing = getBy([&entry](const Notification::Ptr& notif) { return notif->id_ == entry->id_; });

        if(!existing)
            return false;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(entry->owner_ != existing->owner_)
        {
            info_updated = true;
            existing->owner_ = entry->owner_;
        }

        if(entry->text_ != existing->text_)
        {
            info_updated = true;
            existing->text_ = entry->text_;
        }

        if(entry->active_ != existing->active_)
        {
            info_updated = true;
            existing->active_ = entry->active_;
        }

        if(entry->type_ != existing->type_)
        {
            info_updated = true;
            existing->type_ = entry->type_;
        }

        if(entry->tpoints_str_ != existing->tpoints_str_)
        {
            info_updated = true;
            existing->schedule_ = entry->schedule_;
            existing->tpoints_str_ = entry->tpoints_str_;
            existing->wdays_str_ = entry->wdays_str_;
        }

        if(entry->wdays_str_ != existing->wdays_str_)
        {
            info_updated = true;
            existing->schedule_ = entry->schedule_;
            existing->wdays_str_ = entry->wdays_str_;
        }

        if(entry->added_on_ != existing->added_on_)
        {
            info_updated = true;
            existing->added_on_ = entry->added_on_;
        }

        if(entry->expiring_on_ != existing->expiring_on_)
        {
            info_updated = true;
            existing->expiring_on_ = entry->expiring_on_;
        }

        if(entry->parse_mode_ != existing->parse_mode_)
        {
            info_updated = true;
            existing->parse_mode_ = entry->parse_mode_;
        }

        if(!info_updated)
            return false;
    }

    file_->sendQuery(
                (std::string)"UPDATE notifications SET owner='" + std::string(entry->owner_)
                + std::string("', text='") + std::string(entry->text_)
                + std::string("', active=") + std::to_string(entry->active_)
                + std::string(", type=") + std::to_string(static_cast<int>(entry->type_))
                + std::string(", tpoints='") + entry->tpoints_str_
                + std::string("', wdays='") + entry->wdays_str_
                + std::string("', added_on=") + std::to_string(entry->added_on_)
                + std::string(", expiring_on=") + std::to_string(entry->expiring_on_)
                + std::string(", parse_mode='") + entry->parse_mode_
                + std::string("' WHERE id=") + std::to_string(entry->id_)
            );

    Logger::write(": INFO : DATABASE : Notification [" + std::to_string(entry->id_) + "] [" + entry->owner_ + "] has been updated.");

    return true;
}

void NotificationTable::sync() const
{
    auto f = [this](const Notification::Ptr& entry)
    {
        file_->sendQuery(
                    (std::string)"UPDATE notifications SET owner='" + std::string(entry->owner_)
                    + std::string("', text='") + std::string(entry->text_)
                    + std::string("', active=") + std::to_string(entry->active_)
                    + std::string(", type=") + std::to_string(static_cast<int>(entry->type_))
                    + std::string(", tpoints='") + entry->tpoints_str_
                    + std::string("', wdays='") + entry->wdays_str_
                    + std::string("', added_on=") + std::to_string(entry->added_on_)
                    + std::string(", expiring_on=") + std::to_string(entry->expiring_on_)
                    + std::string(", parse_mode='") + entry->parse_mode_
                    + std::string("' WHERE id=") + std::to_string(entry->id_)
                );
    };
    forRange(f);

    Logger::write(": INFO : DATABASE : Notifications have been synced with the SQL base.");
}

void NotificationTable::showTable(std::ostream& os) const noexcept
{
    os << std::endl
       << std::left << std::setw(6) << "ID"
       << std::setw(8) << "ACTIVE"
       << std::setw(8) << "TYPE"
       << std::setw(18) << "OWNER"
       << std::setw(18) << "TEXT"
       << std::setw(18) << "PARSE MODE"
       << std::setw(18) << "SCHEDULE"
       << "ADDED ON"
       << "\t\t\t" << "EXPIRING ON"
       << std::endl;

    auto f = [&os](const Notification::Ptr& entry)
    {
        std::tm added_on_ = localtimeTs(entry->added_on_);
        std::tm expiring_on_ = localtimeTs(entry->expiring_on_);
        os << std::left
           << std::setw(6) << std::to_string(entry->id_)
           << std::setw(8) << (entry->active_ ? "Yes" : "No")
           << std::setw(8) << std::to_string(static_cast<int>(entry->type_))
           << std::setw(18) << AUX::shortenString(entry->owner_, 16)
           << std::setw(18) << AUX::shortenString(entry->text_, 16)
           << std::setw(18) << AUX::shortenString(entry->parse_mode_, 16)
           << std::setw(18) << AUX::shortenString(entry->tpoints_str_, 16)
           << std::put_time(&added_on_, "%d-%m-%Y %H:%M:%S")
           << "\t\t" << std::put_time(&expiring_on_, "%d-%m-%Y %H:%M:%S")
           << std::endl;
    };

    forRange(f);
}

// VPSBASE


VPSTable::VPSTable(const Table<VPS>::SptrF& file, int interval) : Table<VPS>(file, interval)
{
    {
        std::lock_guard<std::mutex> lock(mtx_vec_);
        file_->sendQuery
                (
                    "CREATE TABLE IF NOT EXISTS vps (id INTEGER PRIMARY KEY AUTOINCREMENT,owner INTEGER,uuid TEXT UNIQUE,name BLOB,address TEXT, login TEXT, password TEXT);"
                    "SELECT * FROM vps",
                    extract_vps,
                    &vec_
                );
    }

    Logger::write(": INFO : DATABASE : VPSTable has been initialized.");
}

bool VPSTable::add(const VPS::Ptr& entry)
{
    if(!Table<VPS>::add(entry))
        return false;


    file_->sendQuery(
        (std::string)"INSERT INTO vps (owner, uuid, name, address, login, password) VALUES ("
        + std::to_string(entry->owner_)
        + std::string(", '")
        + entry->uuid_
        + std::string("', '")
        + entry->name_
        + std::string("', '")
        + entry->address_
        + std::string("', '")
        + entry->login_
        + std::string("', '")
        + entry->password_
        + std::string("');"));

    Logger::write(": INFO : DATABASE : New VPS [" + std::to_string(entry->id_) + "] [" + std::to_string(entry->owner_) + "] has been added.");

    return true;
}

bool VPSTable::update(const VPS::Ptr& entry) noexcept
{
    // VECTOR MUTEX SCOPE LOCK
    {
        std::lock_guard<std::mutex> lock_vec(mtx_vec_);

        // Searching for the VPS in the vector.

        auto existing = getBy([&entry](const VPS::Ptr& vps) { return vps->id_ == entry->id_; });

        if(!existing)
            return false;

        bool info_updated = false;

        // Updating the entry in the vector.

        if(entry->owner_ != existing->owner_)
        {
            info_updated = true;
            existing->owner_ = entry->owner_;
        }

        if(entry->uuid_ != existing->uuid_)
        {
            info_updated = true;
            existing->uuid_ = entry->uuid_;
        }

        if(entry->address_ != existing->address_)
        {
            info_updated = true;
            existing->address_ = entry->address_;
        }

        if(entry->login_ != existing->login_)
        {
            info_updated = true;
            existing->login_ = entry->login_;
        }

        if(entry->password_ != existing->password_)
        {
            info_updated = true;
            existing->password_ = entry->password_;
        }

        if(entry->name_ != existing->name_)
        {
            info_updated = true;
            existing->name_ = entry->name_;
        }



        if(entry->state_ != existing->state_)
            existing->state_ = entry->state_;

        if(entry->last_output_ != existing->last_output_ && entry->last_output_ != R"(👁‍ _Awaiting new actions\.\.\._)")
            existing->last_output_ = entry->last_output_;

        if(entry->blocks_ != existing->blocks_)
            existing->blocks_ = entry->blocks_;

        if(entry->netifstat_ != existing->netifstat_)
            existing->netifstat_ = entry->netifstat_;

        if(!info_updated)
            return false;
    }

    file_->sendQuery(
                (std::string)"UPDATE vps SET owner=" + std::to_string(entry->owner_)
                + std::string(", uuid='") + entry->uuid_
                + std::string("', name='") + entry->name_
                + std::string("', address='") + entry->address_
                + std::string("', login='") + entry->login_
                + std::string("', password='") + entry->password_
                + std::string("' WHERE id=") + std::to_string(entry->id_)
            );

    Logger::write(": INFO : DATABASE : A VPS [" + std::to_string(entry->id_) + "] [" + std::to_string(entry->owner_) + "] has been updated.");

    return true;
}

void VPSTable::sync() const
{
    auto f = [this](const VPS::Ptr& entry)
    {
        file_->sendQuery(
                    (std::string)"UPDATE vps SET owner=" + std::to_string(entry->owner_)
                    + std::string(", uuid='") + entry->uuid_
                    + std::string("', name='") + entry->name_
                    + std::string("', address='") + entry->address_
                    + std::string("', login='") + entry->login_
                    + std::string("', password='") + entry->password_
                    + std::string("' WHERE id=") + std::to_string(entry->id_)
                );
    };
    forRange(f);

    Logger::write(": INFO : DATABASE : VPS base have been synced with the SQL base.");
}

void VPSTable::showTable(std::ostream& os) const noexcept
{
    os << std::endl
       << std::left << std::setw(6) << "ID"
       << std::setw(18) << "OWNER"
       << std::setw(18) << "UUID"
       << std::setw(18) << "NAME"
       << std::setw(18) << "ADDRESS"
       << std::setw(18) << "LOGIN"
       << std::setw(18) << "PASSWORD"
       << std::endl;

    auto f = [&os](const VPS::Ptr& entry)
    {
        os << std::left
           << std::setw(6) << std::to_string(entry->id_)
           << std::setw(18) << AUX::shortenString(std::to_string(entry->owner_), 16)
           << std::setw(18) << AUX::shortenString(entry->uuid_, 16)
           << std::setw(18) << AUX::shortenString(entry->name_, 16)
           << std::setw(18) << AUX::shortenString(entry->address_, 16)
           << std::setw(18) << AUX::shortenString(entry->login_, 16)
           << std::setw(18) << AUX::shortenString(entry->password_, 16)
           << std::endl;
    };

    forRange(f);
}
