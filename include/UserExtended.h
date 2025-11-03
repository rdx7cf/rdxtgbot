#ifndef USEREXTENDED_H
#define USEREXTENDED_H

#include <bitset>
#include <string>
#include <tgbot/tgbot.h>

#include "BotAction.h"

class UserExtended : public TgBot::User
{
public:
    using Ptr = std::shared_ptr<UserExtended>;

    unsigned active_tasks_; // This bitmask remembers if user has some loop task active before the bot shutdown (one bit for each task).
    std::time_t member_since_;
    BotAction::List::Ptr pending_actions_;


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
            unsigned bset = 0b0000,
            std::int64_t ms = std::time(nullptr),
            const BotAction::List::Ptr& pending_actions = std::make_shared<BotAction::List>())
        :
        active_tasks_(bset),
        member_since_(ms),
        pending_actions_(pending_actions)

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
        : TgBot::User(*tgu), active_tasks_(bset), member_since_(ms) {}

    bool operator==(const UserExtended&) const;
};

#endif
