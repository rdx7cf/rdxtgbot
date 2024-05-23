#pragma once

#include <bitset>
#include <string>

#include <tgbot/tgbot.h>



class UserExtended : public TgBot::User
{
public:
    typedef std::shared_ptr<UserExtended> Ptr;

    UserExtended();
    UserExtended(const TgBot::User::Ptr&);

    std::bitset<4> active_tasks_; // For the future: this bitmask remembers if user had some loop task active before the bot shutdown (one bit for each task).
};
