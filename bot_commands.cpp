#include <fstream>
#include <ios>

#include "bot_commands.h"

void anymsg(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    if(bot.getApi().blockedByUser(message->chat->id))
        return;

    std::string log_message = std::string(" : INFO : BOT : [") + std::to_string(message->from->id) + "] " + message->from->username + " has just wrote: '" + message->text + "'.\n";

    to_filelog(log_message, "./logs/log.log");

    if (message->text.starts_with("/start"))
        return;
}

void start(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    if(bot.getApi().blockedByUser(message->chat->id))
        return;

    auto current_user = message->from;

    std::jthread(
        [&current_user, &database, &bot, &message]()
        {
            if(!database->contains(current_user))
            {
                bot.getApi().sendMessage(message->chat->id, std::string("Hello, Mr. / Mrs. ") + current_user->firstName + "!");
                database->user_add(current_user);
            }
            else
            {
                bot.getApi().sendMessage(message->chat->id, "Haven't we already met?");
                //database->update...
            }
        }
        );

}

