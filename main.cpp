#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <thread>
#include <sstream>


#include <tgbot/tgbot.h>

#include "database.h"
#include "bot_commands.h"
#include "to_filelog.h"
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

    std::unique_ptr<Database> database(new Database(argv[2], to_filelog));

    MyHttpClient mHC;

    TgBot::Bot bot(argv[1], mHC);



    std::jthread long_polling(thread_long_polling, std::ref(bot), std::cref(database));

    std::cout << "Bot username: " << bot.getApi().getMe()->username << std::endl;
    std::cout << "Send 4 to quit: ";
    int choice;
    while(true)
    {
        if(!(std::cin >> choice))
        {

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


            std::cout << "Correct answers only: ";

        }
        switch(choice)
        {
        case 4:
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

            //to_filelog(": INFO : BOT : Long poll has been started.");
            longPoll.start();
        }

    }
    catch (const std::exception& e)
    {
        to_filelog(std::string(": ERROR : BOT : ") + e.what() + ".");
    }

}
