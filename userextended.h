#pragma once

#include <bitset>
#include <string>

#include <tgbot/tgbot.h>



class UserExtended : public TgBot::User
{
public:
    using Ptr = std::shared_ptr<UserExtended>;

    UserExtended(std::int64_t ms = std::time(nullptr), unsigned long bset = 0b0000) : member_since(ms), activeTasks(bset) {}
    UserExtended(const TgBot::User::Ptr& tgu, std::time_t ms = std::time(nullptr), unsigned long bset = 0b0000) : TgBot::User(*tgu), member_since(ms), activeTasks(bset) {}

    std::time_t member_since;
    std::bitset<4> activeTasks; // For the future: this bitmask remembers if user had some loop task active before the bot shutdown (one bit for each task).
};
