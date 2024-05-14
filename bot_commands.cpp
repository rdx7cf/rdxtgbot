#include <fstream>
#include <ios>

#include "bot_commands.h"


// AUX SECTION OPEN

static bool compare(const TgBot::User::Ptr& x, const TgBot::User::Ptr& y)
{
    return x->id == y->id;
}

// AUX SECTION CLOSE

void anymsg(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, std::vector<TgBot::User::Ptr>& users)
{
    if(bot.getApi().blockedByUser(message->chat->id))
        return;

    std::string log_message = std::string(" : INFO : BOT : [") + std::to_string(message->from->id) + "] " + message->from->username + " has just wrote: '" + message->text + "'.\n";

    to_filelog(log_message);

    if (message->text.starts_with("/start"))
        return;



    bot.getApi().sendMessage(message->chat->id, "Your message is: " + message->text);
}

void start(const TgBot::Message::Ptr& message, const TgBot::Bot& bot, std::vector<TgBot::User::Ptr>& users)
{
    if(bot.getApi().blockedByUser(message->chat->id))
        return;

    auto current_user = message->from;

    auto comp = std::bind(compare, std::placeholders::_1, current_user); // originally binder1st; it transforms a binary predicate compare to a unary.
    auto existing_user_It = find_if(users.begin(), users.end(), comp);

    if(existing_user_It == users.end())
    {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
        users.push_back(current_user);

    }
}

