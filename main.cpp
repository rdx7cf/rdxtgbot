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


int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "\nUSAGE: tgbot -T '[API_TOKEN]' -D '[PATH_TO_DATABASE]' -L '[LOG_PATH]'\n\n"
                     "-T '[API_TOKEN]'\n\n\tAn API token for your bot.\n\n\n "
                     "-D '[PATH_TO_DATABASE]'\n\n\tA path to a SQLite3 database file.\n\tThe program will create one (but not directories) if the specified doesn't exist.\n\n"
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
        std::cout << "\nAVAILABLE COMMANDS:\n1. Show users table (short version);\n2. Send a message to a user;\n3. Send a message to all users;\n4. Sync the database with the file;\n5. Quit.\nEnter a number: ";
        std::cin >> choice;


        if(std::cin.eof()) // No occasional EOF.
        {
            bot.database_->sync();
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
            bot.database_->show_table(std::cout);
            break;
        case 2:
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

            if(!bot.database_->contains(user_id))
            {
                std::cout << "There's no user with such id '" << user_id << "'.";
                break;
            }

            std::cout << "Enter a message for the user: ";
            std::getline(std::cin, message);

            bot.notify_one(user_id, message);

            break;
        }
        case 3:
        {
            std::string message;
            std::cout << "Enter a message for all users: ";
            std::getline(std::cin, message);

            bot.notify_all(message);

            break;
        }
        case 4:
            bot.database_->sync();
            std::cout << "The database is saved to '" << filename << "'; the backup is '" << filename << ".bak'.\n";
            break;
        case 5:
            bot.database_->sync();
            bot.notify_all("It seems we're saying goodbye...");
            return 0;
        }
    }
}
