#include "multithreading.h"

void thread_long_polling(std::stop_token tok, TgBot::Bot& bot, std::vector<TgBot::User::Ptr>& users)
{
    bot.getEvents().onAnyMessage([&users, &bot](TgBot::Message::Ptr message)
                                 { anymsg(message, bot, users); });

    bot.getEvents().onCommand("start",
                              [&users, &bot](TgBot::Message::Ptr message)
                              { start(message, bot, users); });


    TgBot::TgLongPoll longPoll(bot);

    while (true)
    {
        if(tok.stop_requested())
        {
            //std::cout << "Stop requested." << std::endl;
            return;
        }
        try
        {
            longPoll.start();
        }
        catch (TgBot::TgException& e)
        {
            //std::cerr << "ERROR " << int(e.errorCode) << " : " << e.what() << std::endl;
        }
    }

}



void thread_db_syncing(std::stop_token tok)
{

}
