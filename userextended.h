#pragma once

#include <bitset>
#include <string>

#include <tgbot/tgbot.h>



class UserExtended : public TgBot::User
{
public:
    typedef std::shared_ptr<UserExtended> Ptr;

    UserExtended(bool b = false, std::int64_t ms = 0) : blocked(b), member_since(ms) {}
    UserExtended(const TgBot::User::Ptr&, bool block = false, std::time_t ms = 0);

    std::bitset<4> activeTasks; // For the future: this bitmask remembers if user had some loop task active before the bot shutdown (one bit for each task).
    bool blocked;
    std::time_t member_since;
};
