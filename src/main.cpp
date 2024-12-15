#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <thread>
#include <sstream>
#include <iomanip>
#include <csignal>


#include <tgbot/tgbot.h>

#include "Table.h"
#include "SQLFile.h"
#include "MyHTTPclient.h"
#include "Logger.h"
#include "BotExtended.h"
#include "Notification.h"

int enter_number(std::istream&, std::ostream&);

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << R"(USAGE: tgbot -T '[API_TOKEN]' -D '[PATH_TO_DATABASE]' -L '[LOG_PATH]'

-T '[API_TOKEN]'

    An API token for your bot.

-D '[PATH_TO_DATABASE]'

    A path to a SQLite3 database file which contains user and ad tables.
    The program will create one (but not the directories) if the specified doesn't exist.

-L '[LOG_PATH]'

    A path to a log file.
    The program will create one (but not directories) if the specified doesn't exist.
    Might be omitted.

-S [SECONDS]

    Enables auto-sync of internal and external storages.
    The program uses a vector for fast access and a SQLite3 database for long-term storage.
    Might be omitted.

-C [COPY_COUNT]

    Set count of SQL file backup copies.
    Default is 5.
    Might be omitted.
)";
        return 1;
    }

    std::cout << "\nINITIALIZING...\n";

    std::string bot_token;
    std::string db_path;
    std::int32_t interval = -1;
    int copies_counter = 5;

    std::vector<std::string> params(argv, argv + argc);
    // SET API TOKEN
    auto it = std::find(params.begin(), params.end(), "-T");

    if(it != params.end())
        bot_token = *(++it);
    else
        throw std::runtime_error("No API token specified!");


    // SET DATABASE FILE
    it = std::find(params.begin(), params.end(), "-D");

    if(it != params.end())
        db_path = *(++it);
    else
        throw std::runtime_error("No Database file specified!");

    // SET COPIES COUNTER
    it = std::find(params.begin(), params.end(), "-C");

    if(it != params.end())
        copies_counter = std::stoi(*(++it));

    // SET AUTO_SYNC
    it = std::find(params.begin(), params.end(), "-S");

    if(it != params.end())
        interval = std::stoi(*(++it));
    else
        std::cout << "** Loop sync is DISABLED." << std::endl;


    MyHttpClient mhc;
    std::shared_ptr<SQLFile> file = std::make_shared<SQLFile>(db_path, copies_counter, interval);
    UserTable::Ptr usertable_ptr = std::make_shared<UserTable>(file, interval);
    NotificationTable::Ptr notificationtable_ptr = std::make_shared<NotificationTable>(file, interval);
    VPSTable::Ptr vpstable_ptr = std::make_shared<VPSTable>(file, interval);
    BotExtended::Ptr bot = std::make_shared<BotExtended>(bot_token, mhc, usertable_ptr, notificationtable_ptr, vpstable_ptr);

    // SET LOG_FILE
    it = std::find(params.begin(), params.end(), "-L");

    if(it != params.end())
        Logger::filename_ = *(++it);
    else
        std::cout << "** No log file specified, using the default file: './log.log'." << std::endl;

    Logger::write("-------------------");
    Logger::write("BOT INITIALIZING...");
    Logger::write("-------------------");

    // Using std::bind is a workaround for GCC10.
    std::jthread longPolling(&BotExtended::longPolling, bot);

    std::jthread auto_syncing_users(&Table<UserExtended>::autoSync, usertable_ptr);
    std::jthread auto_syncing_notif(&Table<Notification>::autoSync, notificationtable_ptr);
    std::jthread auto_syncing_vps(&Table<VPS>::autoSync, vpstable_ptr);

    std::jthread auto_backuping(&SQLFile::autoBackup, file);
    std::jthread announcing(&BotExtended::announcing, bot);


    signal(SIGINT, SIG_IGN); // No occasional ctrl + C.

    std::time_t now = std::time(nullptr);    
    std::cout << "\nBOT HAS BEEN INITIALIZED ON: " << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << std::endl;
    std::cout << "BOT USERNAME: " << bot->getApi().getMe()->username << '\t' << "BOT ID: " << bot->getApi().getMe()->id << std::endl;

    Logger::write("-------------------");
    Logger::write("BOT INITIALIZED.");
    Logger::write("-------------------");

    while(true)
    {
        std::cout << R"(
AVAILABLE COMMANDS:
1. Show users table;                    2. Show notifications table;
3. Send a message to a user;            4. Send a message to all users;
5. Add a notification;                  6. Update a notification;
7. Edit user's active tasks bitmask;    8. Add a VPS entry;
9. Update a VPS entry;                  10. Show VPS table.
11. Quit.
Enter a number: )";

        switch(enter_number(std::cin, std::cout))
        {
        case INT_MAX:
            usertable_ptr->sync();
            notificationtable_ptr->sync();
            vpstable_ptr->sync();
            bot->notifyAll("It seems we're saying goodbye...");
            return 0;
        case 1:
            usertable_ptr->showTable(std::cout);
            break;
        case 2:
            notificationtable_ptr->showTable(std::cout);
            break;
        case 3:
        {
            std::int64_t user_id;
            std::string message;
            std::cout << "\n<SENDING A MESSAGE TO A USER>\n";
            std::cout << "Enter user's Telegram ID: ";
            user_id = enter_number(std::cin, std::cout);

            auto user = usertable_ptr->getCopyBy([&user_id](const Table<UserExtended>::SptrT& entry) { return entry->id == user_id; });
            if(user)
            {
                std::cout << "Enter a message for the user (CTRL+D to send): ";
                for(std::string temp; std::getline(std::cin, temp); )
                    message += temp + '\n';
                clearerr(stdin);
                std::cin.clear();

                bot->notifyOne(user_id, message);
            }
            else
                std::cout << "There's no user with this id_.";

            break;
        }
        case 4:
        {
            std::string message;
            std::cout << "\n<SENDING A MESSAGE TO ALL USERS>\n";
            std::cout << "Enter a message for all users (CTRL+D to send): ";
            for(std::string temp; std::getline(std::cin, temp); )
                message += temp + '\n';
            clearerr(stdin);
            std::cin.clear();

            bot->notifyAll(message);

            break;
        }
        case 5:
        {
            Notification::Ptr notif = std::make_shared<Notification>(notificationtable_ptr->getLastId() + 1);
            std::tm t;

            std::cout << "\n<ADDING A NOTIFICATION>\n";

            std::cout << "Enter the owner's name: ";
            std::getline(std::cin, notif->owner_);

            std::cout << "Enter the text: \n";

            for(std::string temp; std::getline(std::cin, temp); )
                notif->text_ += temp + '\n';

            clearerr(stdin);
            std::cin.clear();

            std::cout << "ON / OFF (1 / 0): ";
            notif->active_ = enter_number(std::cin, std::cout);

            std::cout << "TYPE: (-1, 0, 1) : SYSTEM, COMMERCIAL, CURRENCY :";
            notif->type_ = static_cast<Notification::TYPE>(enter_number(std::cin, std::cout));

            std::cout << "Enter the expiration date (D-M-Y H:M:S): ";
            std::cin >> std::get_time(&t, "%d-%m-%Y %H:%M:%S");

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::cout << "Enter timepoints ('15:30 17:30 00:00 01:05'): ";
            std::getline(std::cin, notif->tpoints_str_);

            std::cout << "Enter weekdays ('0 1 2 3 4 5 6', where 0 is Sunday and 6 is Saturday): ";
            std::getline(std::cin, notif->wdays_str_);

            notif->schedule_ = extractSchedule(notif->tpoints_str_, notif->wdays_str_);
            if(notif->schedule_.size() == 0)
            {
                Logger::write(": WARN : DATABASE : No schedule specified for: '" + notif->owner_ + "'.");
                notif->active_ = false;
                notif->tpoints_str_.clear();
                notif->wdays_str_.clear();
            }

            notif->added_on_ = static_cast<std::int64_t>(std::time(nullptr));
            notif->expiring_on_ = static_cast<std::int64_t>(mktime(&t));

            notificationtable_ptr->add(notif);

            break;
        }
        case 6:
        {
            std::cout << "\n<UPDATING A NOTIFICATION>\n";
            std::cout << "Enter an id_ of a notification to update: ";

            auto notif = notificationtable_ptr->getCopyBy([](const Notification::Ptr& entry) { return entry->id_ == enter_number(std::cin, std::cout); }); // Какой же здесь ад происходит...
            if(!notif)
            {
                std::cout << "There's no notification with this id_.\n\n";
                break;
            }
            std::cout << "Choose a field to update:\n"
                         "1. Owner name;\t\t2. Text;\n"
                         "3. On/Off;\t\t4. Switch ad. status;\n"
                         "5. Timepoints;\t6. Expiration date;\n"
                         "7. Weekdays;\t\t8. Quit updating.\n"
                         "Enter a number: ";
            switch(enter_number(std::cin, std::cout))
            {
            case INT_MAX:
            {
                usertable_ptr->sync();
                notificationtable_ptr->sync();
                vpstable_ptr->sync();
                bot->notifyAll("It seems we're saying goodbye...");
                return 0;
            }
            case 1:
            {
                std::cout << "Enter a new name (previous: " + notif->owner_ + "): ";
                std::getline(std::cin, notif->owner_);
                break;
            }
            case 2:
            {
                notif->text_ = std::string();
                std::cout << "Enter the text: \n";
                for(std::string temp; std::getline(std::cin, temp); )
                    notif->text_ += temp + '\n';
                clearerr(stdin);
                std::cin.clear();
                break;
            }
            case 3:
            {
                std::cout << "ON / OFF (1 / 0) (previous: " + std::to_string(notif->active_) + ": ";
                notif->active_ = enter_number(std::cin, std::cout);
                break;
            }
            case 4:
            {
                std::cout << "TYPE: (-1, 0, 1) : SYSTEM, COMMERCIAL, CURRENCY :";
                notif->type_ = static_cast<Notification::TYPE>(enter_number(std::cin, std::cout));
                break;
            }
            case 5:
            {
                std::cout << "Enter the time schedule_ ('15:30 17:30 00:00 01:05'): ";
                std::getline(std::cin, notif->tpoints_str_);
                notif->schedule_ = extractSchedule(notif->tpoints_str_, notif->wdays_str_);
                if(notif->schedule_.size() == 0)
                {
                    Logger::write(": WARN : DATABASE : No schedule specified for: '" + notif->owner_ + "'.");
                    notif->active_ = false;
                    notif->tpoints_str_.clear();
                    notif->wdays_str_.clear();
                }
                break;
            }
            case 6:
            {
                std::tm t {};
                std::cout << "Enter the expiration date (D-M-Y H:M:S): ";
                std::cin >> std::get_time(&t, "%d-%m-%Y %H:%M:%S");
                notif->expiring_on_ = static_cast<std::int64_t>(mktime(&t));
                break;
            }
            case 7:
            {
                std::cout << "Enter weekdays ('0 1 2 3 4 5 6', where 0 is Sunday and 6 is Saturday): ";
                std::getline(std::cin, notif->wdays_str_);
                notif->schedule_ = extractSchedule(notif->tpoints_str_, notif->wdays_str_);
                if(notif->schedule_.size() == 0)
                {
                    Logger::write(": WARN : DATABASE : No schedule specified for: '" + notif->owner_ + "'.");
                    notif->active_ = false;
                    notif->tpoints_str_.clear();
                    notif->wdays_str_.clear();
                }
                break;
            }

            }
            notificationtable_ptr->update(notif);
            break;
        }
        case 7:
        {
            std::cout << "\n<EDITING USER'S ACTIVE TASKS BITMASK>\n";
            std::cout << "Enter user's Telegram ID: ";
            std::int64_t user_id = enter_number(std::cin, std::cout);
            auto user = usertable_ptr->getCopyBy([&user_id](const UserExtended::Ptr& entry) { return entry->id == user_id; });
            if(user)
            {
                std::string temp;
                std::cout << "Current tasks bitmask: " << user->active_tasks_.to_string() << '\n';
                std::cout << "Enter a new bitmask (0000, 0001, 0011, etc.): ";
                std::getline(std::cin, temp);
                user->active_tasks_ = std::stoul(temp, 0, 2);
                if(usertable_ptr->update(user))
                    usertable_ptr->sync();
            }
            else
                std::cout << "There's no user with this id_.";

            break;
        }
        case 8:
        {
            std::cout << "\n<ADDING A VPS ENTRY>\n";
            std::cout << "Enter the VPS uuid: ";
            std::string uuid;
            std::getline(std::cin, uuid);

            if(uuid.size() != 36)
                std::cout << "The UUID length isn't correct.\n";
            else
            {
                std::cout << "Enter the owner's Telegram ID: ";
                VPS::Ptr vps = std::make_shared<VPS>(uuid, vpstable_ptr->getLastId() + 1, enter_number(std::cin, std::cout));

                vpstable_ptr->add(vps);
            }

            break;
        }
        case 9:
        {
            std::cout << "\n<UPDATING A VPS ENTRY>\n";
            std::cout << "Enter an id_ of a VPS entry to update: ";

            auto vps = vpstable_ptr->getCopyBy([](const VPS::Ptr& entry) { return entry->id_ == enter_number(std::cin, std::cout); });
            if(!vps)
            {
                std::cout << "There's no VPS entry with this id_.\n\n";
                break;
            }
            std::cout << "Choose a field to update:\n"
                         "1. Owner's ID;\t2. VPS UUID;\n"
                         "3. VPS name;\t4. Quit updating."
                         "Enter a number: ";
            switch(enter_number(std::cin, std::cout))
            {
            case INT_MAX:
            {
                usertable_ptr->sync();
                notificationtable_ptr->sync();
                vpstable_ptr->sync();
                bot->notifyAll("It seems we're saying goodbye...");
                return 0;
            }
            case 1:
            {
                std::cout << "Enter new owner's ID: ";
                vps->owner_ = enter_number(std::cin, std::cout);
                break;
            }
            case 2:
            {
                std::cout << "Enter the VPS uuid: \n";
                std::getline(std::cin, vps->uuid_);
                break;
            }
            case 3:
            {
                std::cout << "Enter the VPS name: \n";
                std::getline(std::cin, vps->name_);
                break;
            }
            }
            vpstable_ptr->update(vps);
            break;
        }
        case 10:
            vpstable_ptr->showTable(std::cout);
            break;
        case 11:
            usertable_ptr->sync();
            notificationtable_ptr->sync();
            vpstable_ptr->sync();
            bot->notifyAll("It seems we're saying goodbye...");
            return 0;
        }
    }
}

int enter_number(std::istream& is, std::ostream& os)
{
    int choice;

    while(!(is >> choice))
    {
        if(is.eof()) // No occasional EOF.
        {
            is.clear();
            is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return INT_MAX;
        }

        is.clear();
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        os << "Correct answers only: ";
    }

    is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    return choice;
}
