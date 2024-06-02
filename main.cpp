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
#include "threads.h"
#include "ad.h"


int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "\nUSAGE: tgbot -T '[API_TOKEN]' -D '[PATH_TO_USERBASE]' -A '[PATH_TO_ADBASE]' -L '[LOG_PATH]'\n\n"
                     "-T '[API_TOKEN]'\n\n\tAn API token for your bot.\n\n\n "
                     "-D '[PATH_TO_USERBASE]'\n\n\tA path to a SQLite3 database file which contains user and ad tables.\n\tThe program will create one (but not directories) if the specified doesn't exist.\n\n"
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


    // SET USERBASE FILE
    it = std::find(params.begin(), params.end(), "-D");

    if(it != params.end())
        filename = *(++it);
    else
        throw std::runtime_error("No Userbase file specified!");

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

    std::jthread long_polling(thread_long_polling, std::ref(bot));
    std::jthread auto_syncing(thread_auto_sync, std::cref(bot), std::cref(interval));

    signal(SIGINT, SIG_IGN); // No occasional ctrl + C.

    std::time_t now = std::time(nullptr);    
    std::cout << "\nBOT INITIALIZED ON: " << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << std::endl;
    std::cout << "BOT USERNAME: " << bot.getApi().getMe()->username << '\t' << "BOT ID: " << bot.getApi().getMe()->id << std::endl;

    Logger::write("-------------------");
    Logger::write("BOT INITIALIZED.");
    Logger::write("-------------------");

    int choice;
    while(true)
    {
        std::cout << "\nAVAILABLE COMMANDS:\n1. Show users table (short version);\t2. Show advertisements table (short version);\n3. Send a message to a user;\t4. Send a message to all users;\n5. Add an advertisement;\t6. Sync the userbase with the file;\n7. Quit.\nEnter a number: ";
        std::cin >> choice;


        if(std::cin.eof()) // No occasional EOF.
        {
            bot.userbase_->sync();
            bot.adbase_->sync();
            bot.notify_all("It seems we're saying goodbye...");
            return 0;
        }

        if(std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Correct answers only: ";
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << std::endl;
        switch(choice)
        {
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
            while(!(std::cin >> user_id))
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Correct answers only: ";
            }

            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if(!bot.userbase_->contains(user_id))
            {
                std::cout << "There's no user with such id '" << user_id << "'.";
                break;
            }

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
            std::tm t{};


            std::cout << "Enter the owner: ";
            std::getline(std::cin, ad->owner);

            std::cout << "Enter the text: \n";


            for(std::string temp; std::getline(std::cin, temp); )
                ad->text += temp + '\n';

            clearerr(stdin);
            std::cin.clear();

            std::cout << "Enter the expiration date (Y-m-d H:M:S): ";
            std::cin >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");

            ad->expiring_on = static_cast<std::int64_t>(mktime(&t));

            bot.adbase_->add(ad);

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            break;
        }
        case 6:
            bot.userbase_->sync();
            bot.adbase_->sync(); // Нужно потом сделать так, чтобы синхронизировало их за один раз.
            std::cout << "The userbase is saved to '" << filename << "'; the backup is '" << filename << ".bak'.\n";
            std::cout << "The adbase is saved to '" << filename << "'; the backup is '" << filename << ".bak'.\n";
            break;
        case 7:
            bot.userbase_->sync();
            bot.adbase_->sync();
            bot.notify_all("It seems we're saying goodbye...");
            return 0;
        }
    }
}
