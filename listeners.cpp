#include <fstream>
#include <ios>

#include "listeners.h"

void anymsg(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    if(!bot.database_->contains(message->from))
        std::jthread([&bot, &message](){bot.database_->user_add(UserExtended::Ptr(new UserExtended(message->from)));});
    else
        std::jthread([&bot, &message](){bot.database_->user_update(message->from);});

    std::string log_message = std::string(": INFO : BOT : [") + std::to_string(message->from->id) + "] " + message->from->firstName + " sent '" + message->text + "'.";
    Logger::write(log_message);
}

void noncom(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    bot.getApi().sendMessage(message->chat->id, "They haven't taught me that command.");
}

void start(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    bot.getApi().sendMessage(message->chat->id, "[TEMPLATE ONSTART_INFO]");
}

