#include "botextended.h"

void BotExtended::long_polling(std::stop_token tok)
{
    getEvents().onAnyMessage(
                [this](TgBot::Message::Ptr message)
                                    { anymsg(message, *this); });

    getEvents().onNonCommandMessage(
                [this](TgBot::Message::Ptr message)
    {
        try
        {
            noncom(message, *this);
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    getEvents().onCommand("start",
                [this](TgBot::Message::Ptr message)
    {
        try
        {
            start(message, *this);
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    Logger::write(": INFO : THREADS : Long polling has been initialized.");
    notify_all("I'm alive!");
    TgBot::TgLongPoll longPoll(*this, 100, 1);

    while(!tok.stop_requested())
    {
        try
        {
            longPoll.start();
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    }
    Logger::write(": INFO : THREADS : Long polling has been stopped.");

}

void BotExtended::auto_sync(std::stop_token tok, std::int32_t seconds)
{
    if(seconds < 0)
        return;

    Logger::write(": INFO : THREADS : Loop sync has been started.");

    while(!tok.stop_requested())
    {
        userbase_->sync();
        notifbase_->sync();
        Logger::write(": INFO : DATABASE : Next sync will be in: " + std::to_string(seconds) + " seconds.");
        for(std::int32_t wait = 0; wait < seconds && !tok.stop_requested(); ++wait )
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Logger::write(": INFO : THREADS : Loop sync has been stopped.");
}

void BotExtended::notify_one(std::int64_t user_id, const std::string& message)
{
    try
    {
        getApi().sendMessage(user_id, message);
        Logger::write(": INFO : BOT : User [" + std::to_string(user_id) + "] has received message.");
    }
    catch(const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::notify_all(const std::string& message, bool isAd)
{
    Logger::write(": INFO : BOT : Notifying all users...");

    auto f = [this, &message, &isAd](UserExtended::Ptr& user)
    {
        if(!user->blocked)
        {
            if(isAd && user->activeTasks[0] == 1)
                return;

            notify_one(user->id, message);
        }
        else
            Logger::write(": INFO : BOT : User [" + std::to_string(user->id) + "] blocked the bot.");
    };
    userbase_->for_range(f);

    Logger::write(": INFO : BOT : Users has been notified.");
}

void BotExtended::advertising(std::stop_token tok)
{
    std::time_t current_timestamp;
    std::tm current;

    auto f = [this, &current, &current_timestamp](Notification::Ptr& notif)
    {
        std::for_each(notif->schedule.begin(), notif->schedule.end(), [this, &current, &current_timestamp, &notif](TmExtended& time_point)
        {
            if(current.tm_wday == time_point.tm_wday && notif->active)
            {
                if(current_timestamp >= notif->expiring_on)
                {
                    notif->active = false;
                    return;
                }
                if (((current.tm_hour == time_point.tm_hour && current.tm_min >= time_point.tm_min) || current.tm_hour > time_point.tm_hour) && !time_point.executed) // Такое монструозное условие нужно для того, чтобы учитывалась разница и между часами, и между часами:минутами (то есть чтобы временная точка типа 15:30 также была допустима)
                {
                    notify_all(notif->text, notif->is_ad);
                    time_point.executed = true;
                }
                else if((current.tm_hour < time_point.tm_hour || (current.tm_hour == time_point.tm_hour && current.tm_min < time_point.tm_min)) && time_point.executed)
                {
                    time_point.executed = false;
                }
            }
        });
    };

    while(!tok.stop_requested())
    {
        current_timestamp = std::time(nullptr);
        current = localtime_ts(current_timestamp);

        notifbase_->for_range(f);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Listeners

void anymsg(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    UserExtended::Ptr uptr(new UserExtended(message->from, bot.getApi().blockedByUser(message->chat->id)));

    if(!bot.userbase_->add(uptr))
        bot.userbase_->update(uptr);

    std::string log_message = std::string(": INFO : BOT : User [") + std::to_string(message->from->id) + "] [" + message->from->firstName + "] has just sent: '" + message->text + "'.";
    Logger::write(log_message);

}

void noncom(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    bot.getApi().sendMessage(message->chat->id, "They haven't taught me this command yet.");
}

void start(const TgBot::Message::Ptr& message, const BotExtended& bot)
{

    bot.getApi().sendMessage(message->chat->id, "At the moment I'm just an echo bot. They will teach me to do something later.");
}


