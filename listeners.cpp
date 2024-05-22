#include <fstream>
#include <ios>

#include "listeners.h"

void anymsg(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
   // std::jthread([&database, &message](){database->user_update(message->from);});
    std::string log_message = std::string(": INFO : BOT : [") + std::to_string(message->from->id) + "] " + message->from->firstName + " sent '" + message->text + "'.";
    Logger::write(log_message);
}

void noncom(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    bot.getApi().sendMessage(message->chat->id, "They haven't taught me that command.");
}

void start(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    if(bot.getApi().blockedByUser(message->chat->id)) return;

    UserExtended::Ptr current_extended(new UserExtended(message->from));

    bool contains = database->contains(message->from);

    if(!contains)
    {
        bot.getApi().sendMessage(message->chat->id, std::string("Hello, Mr. / Mrs. ") + current_extended->firstName + "!");
        std::jthread([&database, &current_extended](){database->user_add(current_extended);});
    }
    else
        bot.getApi().sendMessage(message->chat->id, "Haven't we already met?");
}

