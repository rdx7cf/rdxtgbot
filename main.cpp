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


#include <tgbot/tgbot.h>

#include "database.h"
#include "bot_commands.h"
#include "myhttpclient.h"
#include "logger.h"

void thread_long_polling(std::stop_token, TgBot::Bot&, const std::unique_ptr<Database>&);
void thread_auto_sync(std::stop_token, const std::unique_ptr<Database>&, const std::int32_t&);

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

    std::string temporary;
    std::int32_t interval = -1;

    std::vector<std::string> params(argv, argv + argc);
    // SET API TOKEN
    auto it = std::find(params.begin(), params.end(), "-T");

    if(it != params.end())
        temporary = *(++it);
    else
        throw std::runtime_error("No API token specified!");

    MyHttpClient mHC;
    TgBot::Bot bot(temporary, mHC);

    // SET DATABASE FILE
    it = std::find(params.begin(), params.end(), "-D");

    if(it != params.end())
        temporary = *(++it);
    else
        throw std::runtime_error("No Database file specified!");

    std::unique_ptr<Database> database(new Database(temporary));

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
        std::cout << "** Auto-sync is DISABLED." << std::endl;



    std::jthread long_polling(thread_long_polling, std::ref(bot), std::cref(database));
    std::jthread auto_syncing(thread_auto_sync, std::cref(database), std::cref(interval));

    std::time_t now = std::time(nullptr);    
    std::cout << "\nBOT INITIALIZED ON: " << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << std::endl;
    std::cout << "BOT USERNAME: " << bot.getApi().getMe()->username << '\t' << "BOT ID: " << bot.getApi().getMe()->id << std::endl;

    int choice;
    while(true)
    {
        std::cout << "\nAVAILABLE COMMANDS:\n1. Sync the database with file.\n2. Quit.\nEnter a number: ";
        if(!(std::cin >> choice))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Correct answers only: ";
        }
        switch(choice)
        {
        case 1:
            database->sync();
            std::cout << "The database is saved to '" << argv[2] << "'; the backup is '" << argv[2] << ".bak'.\n";
            break;
        case 2:
            return 0;
        }
    }
}


void thread_long_polling(std::stop_token tok, TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    bot.getEvents().onAnyMessage(
                [&database, &bot](TgBot::Message::Ptr message)
                                    { anymsg(message, bot, database); });

    bot.getEvents().onNonCommandMessage(
                [&database, &bot](TgBot::Message::Ptr message)
                                    { noncom(message, bot, database); });

    bot.getEvents().onCommand("start",
                [&database, &bot](TgBot::Message::Ptr message)
                                    { start(message, bot, database); });


    try
    {
        Logger::write(": INFO : SYSTEM_THREAD : LONG_POLLING : Long polling has been initialized.");
        TgBot::TgLongPoll longPoll(bot, 100, 1);
        while(!tok.stop_requested())
        {
            //Logger::write(": INFO : BOT : Long poll has been started.");
            longPoll.start();
        }
        Logger::write(": INFO : SYSTEM_THREAD : LONG_POLLING : Stop requested.");
    }
    catch (const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }

}

void thread_auto_sync(std::stop_token tok, const std::unique_ptr<Database>& database, const std::int32_t& seconds)
{
    if(seconds < 0)
        return;

    Logger::write(": INFO : SYSTEM_THREAD : AUTO_SYNC : Auto sync has been initialized.");
    while(!tok.stop_requested())
    {
        database->sync();
        Logger::write(": INFO : DATABASE : Database has been synced with file. Waiting for " + std::to_string(seconds) + " seconds before next sync.");
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }
    Logger::write(": INFO : SYSTEM_THREAD : AUTO_SYNC : Stop requested.");
}
