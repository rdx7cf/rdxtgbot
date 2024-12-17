#ifndef BOTEXTENDED_H
#define BOTEXTENDED_H

#include <string>
#include <chrono>

#include <tgbot/tgbot.h>

#include "Table.h"
#include "Notification.h"
#include "VPS.h"

class BotAction;

class BotExtended : public TgBot::Bot
{
public:
    friend class BotAction;
    friend class VPSBotAction;

    using Ptr = std::shared_ptr<BotExtended>;

    const std::chrono::milliseconds LATENCY = std::chrono::milliseconds(150);
    const std::int64_t MASTER = 1373205351;

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
    UserTable::Ptr usertable_;
    NotificationTable::Ptr notificationtable_;
    VPSTable::Ptr vpstable_;

    void vpsHandler(const TgBot::CallbackQuery::Ptr&);
    void vpsProcedure(const TgBot::CallbackQuery::Ptr&, const VPS::Ptr&, VPS::ACTION);
    void vpsInfoEditMessage(const TgBot::CallbackQuery::Ptr&, const VPS::Ptr&, VPS_PAGE, std::string = std::string()) const;
    void vpsInfoEditMessage(const TgBot::Message::Ptr&, const VPS::Ptr&, const TgBot::InlineKeyboardMarkup::Ptr&, std::string = std::string()) const;


};

#endif
