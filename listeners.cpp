#include <fstream>
#include <ios>

#include "listeners.h"

void anymsg(const TgBot::Message::Ptr& message, const BotExtended& bot)
{
    std::jthread([&bot, &message](){bot.userbase_->process_user(UserExtended::Ptr(new UserExtended(message->from)));});

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

    bot.getApi().sendMessage(message->chat->id, "At the moment I'm just an echo bot. They will teach me to do something lately.");
}

