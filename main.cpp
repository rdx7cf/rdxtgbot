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
#include "listeners.h"
#include "myhttpclient.h"
#include "logger.h"
#include "botextended.h"
#include "ad.h"

int enter_number(std::istream&, std::ostream&);

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "\nUSAGE: tgbot -T '[API_TOKEN]' -D '[PATH_TO_DATABASE]' -A '[PATH_TO_ADBASE]' -L '[LOG_PATH]'\n\n"
                     "-T '[API_TOKEN]'\n\n\tAn API token for your bot.\n\n\n "
                     "-D '[PATH_TO_DATABASE]'\n\n\tA path to a SQLite3 database file which contains user and ad tables.\n\tThe program will create one (but not the directories) if the specified doesn't exist.\n\n"
                     "-L '[LOG_PATH]'\n\n\tA path to a log file.\n\tThe program will create one (but not directories) if the specified doesn't exist.\n\tMight be omitted.\n\n"
                     "-S [SECONDS]\n\n\tEnables auto-sync of internal and external storages.\n\tThe program uses a vector for fast access and a SQLite3 database for long-term storage.\n\tMight be omitted.\n\n";
        return 1;
    }

    std::cout << "\nINITIALIZING...\n";

    std::string bot_token;
    std::string filename;
    std::int32_t interval = -1;

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
        filename = *(++it);
    else
        throw std::runtime_error("No Database file specified!");

    MyHttpClient mHC;
    BotExtended bot(bot_token, mHC, filename);

    // SET LOG_FILE
    it = std::find(params.begin(), params.end(), "-L");

    if(it != params.end())
        Logger::filename_ = *(++it);
    else
        std::cout << "** No log file specified, using the default file: './log.log'." << std::endl;


    // SET AUTO_SYNC
    it = std::find(params.begin(), params.end(), "-S");

    if(it != params.end())
        interval = std::stoi(*(++it));
    else
        std::cout << "** Loop sync is DISABLED." << std::endl;


    Logger::write("-------------------");
    Logger::write("BOT INITIALIZING...");
    Logger::write("-------------------");

    std::jthread long_polling(std::bind(&BotExtended::long_polling, &bot, std::placeholders::_1));
    std::jthread auto_syncing(std::bind(&BotExtended::auto_sync, &bot, std::placeholders::_1, std::cref(interval)));
    std::jthread advertising(std::bind(&BotExtended::advertising, &bot, std::placeholders::_1));

    signal(SIGINT, SIG_IGN); // No occasional ctrl + C.

    std::time_t now = std::time(nullptr);    
    std::cout << "\nBOT INITIALIZED ON: " << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << std::endl;
    std::cout << "BOT USERNAME: " << bot.getApi().getMe()->username << '\t' << "BOT ID: " << bot.getApi().getMe()->id << std::endl;

    Logger::write("-------------------");
    Logger::write("BOT INITIALIZED.");
    Logger::write("-------------------");

    while(true)
    {
        std::cout << "\nAVAILABLE COMMANDS:\n1. Show users table;\t2. Show ads table;\n"
                     "3. Send a message to a user;\t4. Send a message to all users;\n"
                     "5. Add an advertisement;\t6. Edit an advertisement;\n"
                     "7. Sync the userbase with the file;\t8. Quit.\n"
                     "Enter a number: "; // Тут можно было бы и raw-формат использовать...

        switch(enter_number(std::cin, std::cout))
        {
        case INT_MAX:
            bot.userbase_->sync();
            bot.adbase_->sync();
            bot.notify_all("It seems we're saying goodbye...");
            return 0;
        case 1:
            bot.userbase_->show_table(std::cout);
            break;
        case 2:
            bot.adbase_->show_table(std::cout);
            break;
        case 3:
        {
            std::int64_t user_id;
            std::string message;

            std::cout << "Enter user's Telegram ID: ";
            user_id = enter_number(std::cin, std::cout);

            std::cout << "Enter a message for the user: ";
            std::getline(std::cin, message);

            bot.notify_one(user_id, message);

            break;
        }
        case 4:
        {
            std::string message;
            std::cout << "Enter a message for all users: ";
            std::getline(std::cin, message);

            bot.notify_all(message);

            break;
        }
        case 5:
        {
            Ad::Ptr ad (new Ad());
            std::tm t;


            std::cout << "Enter the owner's name: ";
            std::getline(std::cin, ad->owner);

            std::cout << "Enter the text: \n";

            for(std::string temp; std::getline(std::cin, temp); )
                ad->text += temp + '\n';

            clearerr(stdin);
            std::cin.clear();

            std::cout << "ON / OFF (1 / 0): ";
            ad->active = enter_number(std::cin, std::cout);

            std::cout << "Enter the expiration date (D-M-Y H:M:S): ";
            std::cin >> std::get_time(&t, "%d-%m-%Y %H:%M:%S");

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::cout << "Enter the time schedule ('15:30 17:30 00:00 01:05'): ";
            std::getline(std::cin, ad->schedule_str);

            ad->schedule = extract_schedule(ad->schedule_str);
            ad->added_on = static_cast<std::int64_t>(std::time(nullptr));
            ad->expiring_on = static_cast<std::int64_t>(mktime(&t));

            bot.adbase_->add(ad);

            break;
        }
        case 6:
        {

            std::cout << "Enter an id of an ad to edit: ";

            Ad::Ptr ad = bot.adbase_->get_copy_by_id(enter_number(std::cin, std::cout)); // Какой же здесь ад происходит...
            if(!ad)
            {
                std::cout << "There's no ad with such id.\n";
                break;
            }
            std::cout << "Choose a field to edit:\n"
                         "1. Owner name;\t2. Text;\n"
                         "3. On/Off;\t4. Schedule;\n"
                         "5.Expiration date;\t6. Quit.\n"
                         "Enter a number: ";
            switch(enter_number(std::cin, std::cout))
            {
            case 1:
                std::cout << "Enter a new name: ";
                std::getline(std::cin, ad->owner);
                break;
            case 2:
                ad->text = std::string();
                std::cout << "Enter the text: \n";
                for(std::string temp; std::getline(std::cin, temp); )
                    ad->text += temp + '\n';
                clearerr(stdin);
                std::cin.clear();
                break;
            case 3:
                std::cout << "ON / OFF (1 / 0): ";
                ad->active = enter_number(std::cin, std::cout);
                break;
            case 4:
                std::cout << "Enter the time schedule ('15:30 17:30 00:00 01:05'): ";
                std::getline(std::cin, ad->schedule_str);
                ad->schedule = extract_schedule(ad->schedule_str);
                break;
            case 5:
                std::tm t {};
                std::cout << "Enter the expiration date (D-M-Y H:M:S): ";
                std::cin >> std::get_time(&t, "%d-%m-%Y %H:%M:%S");
                ad->expiring_on = static_cast<std::int64_t>(mktime(&t));
                break;
            }
            bot.adbase_->update(ad);
            break;
        }
        case 7:
            bot.userbase_->sync();
            bot.adbase_->sync();
            std::cout << "The userbase is saved to '" << filename << "'; the backup is '" << filename << ".bak'.\n";
            std::cout << "The adbase is saved to '" << filename << "'; the backup is '" << filename << ".bak'.\n";
            break;
        case 8:
            bot.userbase_->sync();
            bot.adbase_->sync();
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
    os << std::endl;

    return choice;
}
