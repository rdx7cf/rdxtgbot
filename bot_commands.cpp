#include <fstream>
#include <ios>

#include "bot_commands.h"

void anymsg(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    std::string log_message = std::string(": INFO : BOT : [") + std::to_string(message->from->id) + "] " + message->from->username + " sent command '" + message->text + "'.";

    Logger::write(log_message);

    std::jthread([&database, &message](){database->user_update(message->from);});
}

void noncom(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    std::jthread([&database, &message](){database->user_update(message->from);});

    if (message->text.starts_with("/start"))
        return;

    bot.getApi().sendMessage(message->chat->id, "They haven't taught me that command.");
}

void start(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    auto current_user = message->from;

    bool contains = database->contains(current_user);

    if(!contains)
    {
        bot.getApi().sendMessage(message->chat->id, std::string("Hello, Mr. / Mrs. ") + current_user->firstName + "!");
        std::jthread([&database, &current_user](){database->user_add(current_user);});
    }
    else
        bot.getApi().sendMessage(message->chat->id, "Haven't we already met?");
}

