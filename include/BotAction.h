#ifndef BOTACTION_H
#define BOTACTION_H

#include <memory>
#include <vector>

#include <tgbot/tgbot.h>

#include "VPS.h"

class BotExtended;

class BotAction
{
public:
    using Ptr = std::shared_ptr<BotAction>;

    class List
    {
    public:
        using Ptr = std::shared_ptr<List>;

        void addAction(const BotAction::Ptr&);
        BotAction::Ptr getAction();
        bool isNoActions() const;
        bool cancelActionByMessageId(std::int32_t);

    private:
        std::list<BotAction::Ptr> pending_actions_;
        mutable std::mutex mtx_q_;

    };



    std::vector<TgBot::Message::Ptr> inprogress_messages_;
    const BotExtended* bot_;
    std::string user_input_;

    BotAction(const TgBot::Message::Ptr&,
              const BotExtended*,
              const std::string& = std::string());

    virtual ~BotAction() {}
    void deleteMessages();
    virtual void perform() = 0;
};

class VPSBotAction : public BotAction
{
public:

    using Ptr = std::shared_ptr<VPSBotAction>;

    VPS::Ptr vps_;
    VPS::ACTION action_;

    VPSBotAction(const TgBot::Message::Ptr&,
                 const BotExtended*,
                 const std::string&,
                 const VPS::Ptr&, VPS::ACTION);

    void perform() override;
};

#endif // BOTACTION_H
