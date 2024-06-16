#include "botextended.h"

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
    try
    {
        getApi().sendMessage(user_id, message);
        Logger::write(": INFO : BOT : [" + std::to_string(user_id) + "] RECEIVED MESSAGE.");
    }
    catch(const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::notify_all(const std::string& message)
{
    Logger::write(": INFO : BOT : NOTIFYING ALL...");

    auto f = [this, &message](UserExtended::Ptr& user)
    {
        notify_one(user->id, message);
    };
    userbase_->for_range(f);

    Logger::write(": INFO : BOT : USERS BEEN NOTIFIED.");
}

void BotExtended::advertising(std::stop_token tok)
{
    std::tm current = localtime_ts(std::time(nullptr));

    auto f = [this, &current](Ad::Ptr& ad)
    {
        std::for_each(ad->schedule.begin(), ad->schedule.end(), [this, &current, &ad](TmExtended& time_point)
        {
            if(ad->active)
            {
                if(std::time(nullptr) >= ad->expiring_on)
                {
                    ad->active = false;
                    return;
                }
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
            else if (std::time(nullptr) < ad->expiring_on)
                ad->active = true;
        });
    };

    while(!tok.stop_requested())
    {
        adbase_->for_range(f);

        current = localtime_ts(std::time(nullptr));

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Listeners

void anymsg(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    if(!bot.userbase_->add(UserExtended::Ptr(new UserExtended(message->from))))
        bot.userbase_->update(message->from);

    std::string log_message = std::string(": INFO : BOT : [") + std::to_string(message->from->id) + "] [" + message->from->firstName + "] SENT '" + message->text + "'.";
    Logger::write(log_message);
}

void noncom(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    bot.getApi().sendMessage(message->chat->id, "They haven't taught me this command yet.");
}

void start(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    bot.getApi().sendMessage(message->chat->id, "At the moment I'm just an echo bot. They will teach me to do something later.");
}


