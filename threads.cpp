#include "threads.h"

void thread_long_polling(std::stop_token tok, BotExtended& bot)
{
    bot.getEvents().onAnyMessage(
                [&bot](TgBot::Message::Ptr message)
                                    { anymsg(message, bot); });

    bot.getEvents().onNonCommandMessage(
                [&bot](TgBot::Message::Ptr message)
                                    { noncom(message, bot); });

    bot.getEvents().onCommand("start",
                [&bot](TgBot::Message::Ptr message)
                                    { start(message, bot); });

    Logger::write(": INFO : SYS : LONG POLLING INITALIZED.");
    bot.notify_all("I'm alive!");
    TgBot::TgLongPoll longPoll(bot, 100, 1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while(!tok.stop_requested())
    {
        try
        {
            //Logger::write(": INFO : BOT : .");
            longPoll.start();
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }

    }
    Logger::write(": INFO : SYS : LONG POLLING STOPPED.");

}

void thread_auto_sync(std::stop_token tok, const BotExtended& bot, const std::int32_t& seconds)
{
    if(seconds < 0)
        return;

    Logger::write(": INFO : SYS : LOOP SYNC INITIALIZED.");

    while(!tok.stop_requested())
    {
        bot.userbase_->sync();
        bot.adbase_->sync();
        Logger::write(": INFO : BAS : NEXT SYNC IN: " + std::to_string(seconds) + " SEC.");
        for(std::int32_t wait = 0; wait < seconds && !tok.stop_requested(); ++wait )
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Logger::write(": INFO : SYS : LOOP SYNC STOPPED.");
}

void thread_advertising(std::stop_token tok, BotExtended& bot)
{
    std::tm current = localtime_ts(std::time(nullptr));

    while(!tok.stop_requested())
    {
        std::for_each(bot.adbase_->begin(), bot.adbase_->end(), [&current, &bot](Ad::Ptr& ad)
        {

            std::for_each(ad->schedule.begin(), ad->schedule.end(), [&current, &ad, &bot](TmExtended& time_point)
            {
                if(ad->active)
                {
                    if  (((current.tm_hour == time_point.tm_hour && current.tm_min >= time_point.tm_min) || current.tm_hour > time_point.tm_hour) && !time_point.executed) // Такое монструозное условие нужно для того, чтобы учитывалась разница и между часами, и между часами:минутами (то есть чтобы временная точка типа 15:30 также была допустима)
                    {
                        bot.notify_all(ad->text);
                        time_point.executed = true;
                    }
                    else if((current.tm_hour < time_point.tm_hour || (current.tm_hour == time_point.tm_hour && current.tm_min < time_point.tm_min)) && time_point.executed)
                    {
                        time_point.executed = false;
                    }
                }
            });
        });

        current = localtime_ts(std::time(nullptr));

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
