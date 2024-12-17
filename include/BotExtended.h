#ifndef BOTEXTENDED_H
#define BOTEXTENDED_H

#include <string>
#include <algorithm>
#include <thread>
#include <functional>
#include <iostream>
#include <list>
#include <chrono>

#include <tgbot/tgbot.h>

#include "Table.h"
#include "SQLFile.h"
#include "Logger.h"
#include "BashCommand.h"
#include "UserExtended.h"
#include "Auxiliary.h"

class BotExtended : public TgBot::Bot
{
public:
    using Ptr = std::shared_ptr<BotExtended>;


    static const std::int64_t MASTER = 1373205351;

    enum class VPS_PAGE {MAIN = -1, MANAGE, POWER, BACKUP};

    BotExtended(std::string token,
                const TgBot::HttpClient& http_client,
                const UserTable::Ptr& usertable,
                const NotificationTable::Ptr& notificationtable,
                const VPSTable::Ptr& vpstable,
                const std::string& url = "https://api.telegram.org");

    BotExtended(const BotExtended& bot) : TgBot::Bot(bot.getToken()), usertable_(bot.usertable_), notificationtable_(bot.notificationtable_), vpstable_(bot.vpstable_) {}

    void longPolling(std::stop_token);

    void notifyOne(std::int64_t, const std::string&, const TgBot::GenericReply::Ptr& = nullptr, const std::string& parse_mode = "") const noexcept;

    void notifyAll(const std::string&, Notification::TYPE = Notification::TYPE::SYSTEM, const TgBot::GenericReply::Ptr& = nullptr, const std::string& parse_mode = "") const noexcept;

    void announcing(std::stop_token) const;

    static TgBot::ReplyKeyboardMarkup::Ptr createReply(const std::vector<std::vector<std::string>>&);
    static TgBot::InlineKeyboardMarkup::Ptr createInline(const std::vector<std::vector<std::pair<std::string, std::string>>>&);



private:
    class BotAction // nested classes are friends by default since C++11
    {
    public:
        using Ptr = std::shared_ptr<BotAction>;

        TgBot::User::Ptr owner_;
        std::vector<TgBot::Message::Ptr> inprogress_messages_;
        const BotExtended* bot_;
        std::string user_input_;

        BotAction(const TgBot::User::Ptr&,
                  const TgBot::Message::Ptr&,
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

        VPSBotAction(const TgBot::User::Ptr&,
                     const TgBot::Message::Ptr&,
                     const BotExtended*,
                     const std::string&,
                     const VPS::Ptr&, VPS::ACTION);

        void perform() override;
    };

    std::mutex mtx_actions_;

    UserTable::Ptr usertable_;
    NotificationTable::Ptr notificationtable_;
    VPSTable::Ptr vpstable_;

    std::list<BotAction::Ptr> pending_actions_;

    void vpsHandler(const TgBot::CallbackQuery::Ptr&);
    void vpsProcedure(const TgBot::CallbackQuery::Ptr&, const VPS::Ptr&, VPS::ACTION);
    void vpsInfoEditMessage(const TgBot::CallbackQuery::Ptr&, const VPS::Ptr&, VPS_PAGE, std::string = std::string()) const;
    void vpsInfoEditMessage(const TgBot::Message::Ptr&, const VPS::Ptr&, const TgBot::InlineKeyboardMarkup::Ptr&, std::string = std::string()) const;


};

#endif
