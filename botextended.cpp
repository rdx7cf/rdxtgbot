#include "botextended.h"


void BotExtended::notify_one(const UserExtended::Ptr& user, const std::string& message)
{
    getApi().sendMessage(user->id, message);
    Logger::write(": INFO : BOT : The message sent to the user [" + std::to_string(user->id) + "] " + user->firstName + ".");
}

void BotExtended::notify_all(const std::string& message)
{
    Logger::write(": INFO : BOT : Notifying all the users...");
    std::for_each(database_->users_vec_.begin(), database_->users_vec_.end(),[this, &message](const UserExtended::Ptr& user)
    {
        std::jthread(&BotExtended::notify_one, this, std::cref(user), std::cref(message));
    });
    Logger::write(": INFO : BOT : The users have been notified.");
}
