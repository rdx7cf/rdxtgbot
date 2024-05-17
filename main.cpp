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

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "USAGE:\ntgbot '[API_TOKEN]' '[PATH_TO_DATABASE]' '[LOG_PATH]'\nOR\ntgbot '[API_TOKEN]' '[PATH_TO_DATABASE]'\n";
        return 1;
    }

    if(argc > 3)
        Logger::filename_ = argv[3];

    std::unique_ptr<Database> database(new Database(argv[2]));

    MyHttpClient mHC;

    TgBot::Bot bot(argv[1], mHC);



    std::jthread long_polling(thread_long_polling, std::ref(bot), std::cref(database));

    std::time_t now = std::time(nullptr);
    auto tmp = bot.getApi().getMe();
    std::cout << "BOT INITIALIZED ON: " << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << std::endl;
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
        TgBot::TgLongPoll longPoll(bot, 100, 1);
        while(true)
        {
            if(tok.stop_requested())
            {
                Logger::write(": INFO : SYSTEM : Stop requested.");
                return;
            }

            //Logger::write(": INFO : BOT : Long poll has been started.");
            longPoll.start();
        }

    }
    catch (const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }

}
