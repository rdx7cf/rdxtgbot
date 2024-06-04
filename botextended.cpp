#include "botextended.h"


void BotExtended::notify_one(const std::int64_t& user_id, const std::string& message)
{
    if(userbase_->contains(user_id))
    {
        getApi().sendMessage(user_id, message);
        Logger::write(": INFO : BOT : [" + std::to_string(user_id) + "] RECEIVED MESSAGE.");
    }
    else
    {
        Logger::write(": ERROR : BOT : [" + std::to_string(user_id) + "] UNKNOWN USER.");
    }
}

void BotExtended::notify_all(const std::string& message)
{
    Logger::write(": INFO : BOT : NOTIFYING ALL...");
    std::for_each(userbase_->vec_.begin(), userbase_->vec_.end(),[this, &message](const UserExtended::Ptr& user)
    {
        std::jthread(&BotExtended::notify_one, this, std::cref(user->id), std::cref(message));
    });
    Logger::write(": INFO : BOT : USERS BEEN NOTIFIED.");
}
