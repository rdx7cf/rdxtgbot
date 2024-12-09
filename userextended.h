#pragma once

#include <bitset>
#include <string>
#include <vector>

#include <tgbot/tgbot.h>



class UserExtended : public TgBot::User
{
public:
    using Ptr = std::shared_ptr<UserExtended>;

    std::bitset<4> activeTasks; // This bitmask remembers if user have some loop task active before the bot shutdown (one bit for each task).
    std::time_t member_since;

    UserExtended(
            std::int64_t i,
            const std::string& uname,
            const std::string& fname,
            const std::string& lname,
            const std::string& lcode,
            bool ibot,
            bool iprem,
            bool atam,
            bool cjg,
            bool cragm,
            bool siq,
            unsigned long bset = 0b0000,
            std::int64_t ms = std::time(nullptr))
        :
        activeTasks(bset),
        member_since(ms)
    {
        id = i; username = uname;
        firstName = fname; lastName = lname;
        languageCode = lcode; isBot = ibot;
        isPremium = iprem; addedToAttachmentMenu = atam;
        canJoinGroups = cjg, canReadAllGroupMessages = cragm;
        supportsInlineQueries = siq;
    }


    UserExtended(const TgBot::User::Ptr& tgu,
                 unsigned long bset = 0b0000,
                 std::time_t ms = std::time(nullptr)
                 )
        : TgBot::User(*tgu), activeTasks(bset), member_since(ms) {}


};
