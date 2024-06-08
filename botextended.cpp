#include "botextended.h"
#include "listeners.h"

void BotExtended::long_polling(std::stop_token tok)
{
    getEvents().onAnyMessage(
                [this](TgBot::Message::Ptr message)
                                    { anymsg(message, *this); });

    getEvents().onNonCommandMessage(
                [this](TgBot::Message::Ptr message)
                                    { noncom(message, *this); });

    getEvents().onCommand("start",
                [this](TgBot::Message::Ptr message)
                                    { start(message, *this); });

    Logger::write(": INFO : SYS : LONG POLLING INITALIZED.");
    notify_all("I'm alive!");
    TgBot::TgLongPoll longPoll(*this, 100, 1);
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

void BotExtended::auto_sync(std::stop_token tok, const std::int32_t& seconds)
{
    if(seconds < 0)
        return;

    Logger::write(": INFO : SYS : LOOP SYNC INITIALIZED.");

    while(!tok.stop_requested())
    {
        userbase_->sync();
        adbase_->sync();
        Logger::write(": INFO : BAS : NEXT SYNC IN: " + std::to_string(seconds) + " SEC.");
        for(std::int32_t wait = 0; wait < seconds && !tok.stop_requested(); ++wait )
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Logger::write(": INFO : SYS : LOOP SYNC STOPPED.");
}

void BotExtended::notify_one(const std::int64_t& user_id, const std::string& message)
{
    getApi().sendMessage(user_id, message);
    Logger::write(": INFO : BOT : [" + std::to_string(user_id) + "] RECEIVED MESSAGE.");
}

void BotExtended::notify_all(const std::string& message)
{
    Logger::write(": INFO : BOT : NOTIFYING ALL...");

    std::function<void(UserExtended::Ptr&)> f = [this, &message](UserExtended::Ptr& user)
    {
        notify_one(user->id, message);
    };
    userbase_->for_range(f);

    Logger::write(": INFO : BOT : USERS BEEN NOTIFIED.");
}

void BotExtended::advertising(std::stop_token tok)
{
    std::tm current = localtime_ts(std::time(nullptr));

    while(!tok.stop_requested())
    {
        std::function<void(Ad::Ptr&)> f = [this, &current](Ad::Ptr& ad)
        {
            std::for_each(ad->schedule.begin(), ad->schedule.end(), [this, &current, &ad](TmExtended& time_point)
            {
                if(ad->active)
                {
                    if (((current.tm_hour == time_point.tm_hour && current.tm_min >= time_point.tm_min) || current.tm_hour > time_point.tm_hour) && !time_point.executed) // Такое монструозное условие нужно для того, чтобы учитывалась разница и между часами, и между часами:минутами (то есть чтобы временная точка типа 15:30 также была допустима)
                    {
                        notify_all(ad->text);
                        time_point.executed = true;
                    }
                    else if((current.tm_hour < time_point.tm_hour || (current.tm_hour == time_point.tm_hour && current.tm_min < time_point.tm_min)) && time_point.executed)
                    {
                        time_point.executed = false;
                    }
                }
            });
        };
        adbase_->for_range(f);

        current = localtime_ts(std::time(nullptr));

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
