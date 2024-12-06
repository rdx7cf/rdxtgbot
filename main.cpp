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

#include "database.h"
#include "sqlfile.h"
#include "myhttpclient.h"
#include "logger.h"
#include "botextended.h"
#include "notification.h"

int enter_number(std::istream&, std::ostream&);

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "TEST\nUSAGE: tgbot -T '[API_TOKEN]' -D '[PATH_TO_DATABASE]' -L '[LOG_PATH]'\n\n"
                     "-T '[API_TOKEN]'\n\n\tAn API token for your bot.\n\n\n "
                     "-D '[PATH_TO_DATABASE]'\n\n\tA path to a SQLite3 database file which contains user and ad tables.\n\tThe program will create one (but not the directories) if the specified doesn't exist.\n\n"
                     "-L '[LOG_PATH]'\n\n\tA path to a log file.\n\tThe program will create one (but not directories) if the specified doesn't exist.\n\tMight be omitted.\n\n"
                     "-S [SECONDS]\n\n\tEnables auto-sync of internal and external storages.\n\tThe program uses a vector for fast access and a SQLite3 database for long-term storage.\n\tMight be omitted.\n\n"
                     "-C [COPY_COUNT]\n\n\tSet count of SQL file backup copies.\n\tDefault is 5.\n\tMight be omitted.\n\n";
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
    Userbase::Ptr userbase_ptr = std::make_shared<Userbase>(file, interval);
    Notifbase::Ptr notifbase_ptr = std::make_shared<Notifbase>(file, interval);
    BotExtended bot(bot_token, mhc, userbase_ptr, notifbase_ptr);

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
    std::jthread long_polling(std::bind(&BotExtended::long_polling, &bot, std::placeholders::_1));

    std::jthread auto_syncing_users(std::bind(&Database<UserExtended>::auto_sync, userbase_ptr, std::placeholders::_1));
    std::jthread auto_syncing_notif(std::bind(&Database<Notification>::auto_sync, notifbase_ptr, std::placeholders::_1));

    std::jthread auto_backuping(std::bind(&SQLFile::auto_backup, file, std::placeholders::_1));
    std::jthread announcing(std::bind(&BotExtended::announcing, &bot, std::placeholders::_1, BotExtended::Task::ADS));


    signal(SIGINT, SIG_IGN); // No occasional ctrl + C.

    std::time_t now = std::time(nullptr);    
    std::cout << "\nBOT HAS BEEN INITIALIZED ON: " << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << std::endl;
    std::cout << "BOT USERNAME: " << bot.getApi().getMe()->username << '\t' << "BOT ID: " << bot.getApi().getMe()->id << std::endl;

    Logger::write("-------------------");
    Logger::write("BOT INITIALIZED.");
    Logger::write("-------------------");

    while(true)
    {
        std::cout << "\nAVAILABLE COMMANDS:"
                     "\n1. Show users table;\t\t\t2. Show notifications table;\n"
                     "3. Send a message to a user;\t\t4. Send a message to all users;\n"
                     "5. Add a notification;\t\t\t6. Update a notification;\n"
                     "7. Sync the tables with the file;\t8. Edit user's VPS string;\n"
                     "9. Edit user's active tasks bitmask;\t10. Quit.\n"
                     "Enter a number: "; // Тут можно было бы и raw-формат использовать...

        switch(enter_number(std::cin, std::cout))
        {
        case INT_MAX:
            userbase_ptr->sync();
            notifbase_ptr->sync();
            bot.notify_all("It seems we're saying goodbye...");
            return 0;
        case 1:
            userbase_ptr->show_table(std::cout);
            break;
        case 2:
            notifbase_ptr->show_table(std::cout);
            break;
        case 3:
        {
            std::int64_t user_id;
            std::string message;
            std::cout << "\n<SENDING A MESSAGE TO A USER>\n";
            std::cout << "Enter user's Telegram ID: ";
            user_id = enter_number(std::cin, std::cout);

            UserExtended::Ptr user = userbase_ptr->get_copy_by_id(user_id);
            if(user)
            {
                std::cout << "Enter a message for the user: ";
                std::getline(std::cin, message);

                bot.notify_one(user_id, message);
            }
            else
                std::cout << "There's no user with this id.";

            break;
        }
        case 4:
        {
            std::string message;
            std::cout << "\n<SENDING A MESSAGE TO ALL USERS>\n";
            std::cout << "Enter a message for all users: ";
            std::getline(std::cin, message);

            bot.notify_all(message);

            break;
        }
        case 5:
        {
            Notification::Ptr notif = std::make_shared<Notification>();
            std::tm t;

            std::cout << "\n<ADDING A NOTIFICATION>\n";
            notif->id = notifbase_ptr->get_last_id() + 1;
            std::cout << "Enter the owner's name: ";
            std::getline(std::cin, notif->owner);

            std::cout << "Enter the text: \n";

            for(std::string temp; std::getline(std::cin, temp); )
                notif->text += temp + '\n';

            clearerr(stdin);
            std::cin.clear();

            std::cout << "ON / OFF (1 / 0): ";
            notif->active = enter_number(std::cin, std::cout);

            std::cout << "AD / NOT AD (1 / 0): ";
            notif->is_ad = enter_number(std::cin, std::cout);

            std::cout << "Enter the expiration date (D-M-Y H:M:S): ";
            std::cin >> std::get_time(&t, "%d-%m-%Y %H:%M:%S");

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::cout << "Enter timepoints ('15:30 17:30 00:00 01:05'): ";
            std::getline(std::cin, notif->tpoints_str);

            std::cout << "Enter weekdays ('0 1 2 3 4 5 6', where 0 is Sunday and 6 is Saturday): ";
            std::getline(std::cin, notif->wdays_str);

            notif->schedule = extract_schedule(notif->tpoints_str, notif->wdays_str);
            if(notif->schedule.size() == 0)
            {
                Logger::write(": WARN : DATABASE : No schedule specified for: '" + notif->owner + "'.");
                notif->active = false;
                notif->tpoints_str.clear();
                notif->wdays_str.clear();
            }

            notif->added_on = static_cast<std::int64_t>(std::time(nullptr));
            notif->expiring_on = static_cast<std::int64_t>(mktime(&t));

            notifbase_ptr->add(notif);

            break;
        }
        case 6:
        {
            std::cout << "\n<UPDATING A NOTIFICATION>\n";
            std::cout << "Enter an id of a notification to update: ";

            Notification::Ptr notif = notifbase_ptr->get_copy_by_id(enter_number(std::cin, std::cout)); // Какой же здесь ад происходит...
            if(!notif)
            {
                std::cout << "There's no notification with such id.\n";
                break;
            }
            std::cout << "Choose a field to update:\n"
                         "1. Owner name;\t\t2. Text;\n"
                         "3. On/Off;\t\t4. Switch ad. status;\n"
                         "5. Timepoints;\t6. Expiration date;\n"
                         "7. Weekdays;\t\t8. Quit.\n"
                         "Enter a number: ";
            switch(enter_number(std::cin, std::cout))
            {
            case INT_MAX:
            {
                userbase_ptr->sync();
                notifbase_ptr->sync();
                bot.notify_all("It seems we're saying goodbye...");
                return 0;
            }
            case 1:
            {
                std::cout << "Enter a new name: ";
                std::getline(std::cin, notif->owner);
                break;
            }
            case 2:
            {
                notif->text = std::string();
                std::cout << "Enter the text: \n";
                for(std::string temp; std::getline(std::cin, temp); )
                    notif->text += temp + '\n';
                clearerr(stdin);
                std::cin.clear();
                break;
            }
            case 3:
            {
                std::cout << "ON / OFF (1 / 0): ";
                notif->active = enter_number(std::cin, std::cout);
                break;
            }
            case 4:
            {
                std::cout << "AD / NOT AD (1 / 0): ";
                notif->is_ad = enter_number(std::cin, std::cout);
                break;
            }
            case 5:
            {
                std::cout << "Enter the time schedule ('15:30 17:30 00:00 01:05'): ";
                std::getline(std::cin, notif->tpoints_str);
                notif->schedule = extract_schedule(notif->tpoints_str, notif->wdays_str);
                if(notif->schedule.size() == 0)
                {
                    Logger::write(": WARN : DATABASE : No schedule specified for: '" + notif->owner + "'.");
                    notif->active = false;
                    notif->tpoints_str.clear();
                    notif->wdays_str.clear();
                }
                break;
            }
            case 6:
            {
                std::tm t {};
                std::cout << "Enter the expiration date (D-M-Y H:M:S): ";
                std::cin >> std::get_time(&t, "%d-%m-%Y %H:%M:%S");
                notif->expiring_on = static_cast<std::int64_t>(mktime(&t));
                break;
            }
            case 7:
            {
                std::cout << "Enter weekdays ('0 1 2 3 4 5 6', where 0 is Sunday and 6 is Saturday): ";
                std::getline(std::cin, notif->wdays_str);
                notif->schedule = extract_schedule(notif->tpoints_str, notif->wdays_str);
                if(notif->schedule.size() == 0)
                {
                    Logger::write(": WARN : DATABASE : No schedule specified for: '" + notif->owner + "'.");
                    notif->active = false;
                    notif->tpoints_str.clear();
                    notif->wdays_str.clear();
                }
                break;
            }

            }
            notifbase_ptr->update(notif);
            break;
        }
        case 7:
            userbase_ptr->sync();
            notifbase_ptr->sync();
            std::cout << "The userbase is saved to '" << db_path << "'; the backup is '" << db_path << ".bak'.\n";
            std::cout << "The adbase is saved to '" << db_path << "'; the backup is '" << db_path << ".bak'.\n";
            break;
        case 8:
        {
            std::int64_t user_id;
            std::cout << "\n<EDITING USER'S VPS STRING>\n";
            std::cout << "Enter user's Telegram ID: ";
            user_id = enter_number(std::cin, std::cout);

            UserExtended::Ptr user = userbase_ptr->get_copy_by_id(user_id);
            if(user)
            {
                std::cout << "Current VPS string: " << user->vps_names_str << '\n';
                std::cout << "Enter a new VPS string for the user (SPACE is a delim, empty string to clear the previous): ";
                std::getline(std::cin, user->vps_names_str);

                user->vps_names = StringTools::split(user->vps_names_str, ' ');
                if(userbase_ptr->update(user))
                    userbase_ptr->sync();
            }
            else
                std::cout << "There's no user with this id.";

            break;
        }
        case 9:
        {
            std::int64_t user_id;
            std::cout << "\n<EDITING USER'S ACTIVE TASKS BITMASK>\n";
            std::cout << "Enter user's Telegram ID: ";
            user_id = enter_number(std::cin, std::cout);

            UserExtended::Ptr user = userbase_ptr->get_copy_by_id(user_id);
            if(user)
            {
                std::string temp;
                std::cout << "Current tasks bitmask: " << user->activeTasks.to_string() << '\n';
                std::cout << "Enter a new bitmask (0000, 0001, 0011, etc.): ";
                std::getline(std::cin, temp);
                user->activeTasks = std::stoul(temp, 0, 2);
                if(userbase_ptr->update(user))
                    userbase_ptr->sync();
            }
            else
                std::cout << "There's no user with this id.";

            break;
        }
        case 10:
            userbase_ptr->sync();
            notifbase_ptr->sync();
            bot.notify_all("It seems we're saying goodbye...");
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
