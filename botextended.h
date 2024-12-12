#pragma once

#include <string>
#include <algorithm>
#include <thread>
#include <functional>
#include <iostream>
#include <type_traits>

#include <tgbot/tgbot.h>

#include "database.h"
#include "sqlfile.h"
#include "logger.h"
#include "bashcommand.h"
#include "userextended.h"
#include "auxiliary.h"

class BotExtended : public TgBot::Bot
{
public:
    static const std::int64_t MASTER = 1373205351;

    enum class PAGE {MAIN = -1, MANAGE, POWER, BACKUP};

    Userbase::Ptr userbase_;
    Notifbase::Ptr notifbase_;
    VPSbase::Ptr vpsbase_;

    BotExtended(std::string token,
                const TgBot::HttpClient& httpClient,
                const Userbase::Ptr& userbase,
                const Notifbase::Ptr& notifbase,
                const VPSbase::Ptr& vpsbase,
                const std::string& url = "https://api.telegram.org");

    void long_polling(std::stop_token);

    void notify_one(std::int64_t, const std::string&, const TgBot::GenericReply::Ptr& = nullptr) const noexcept;

    void notify_all(const std::string&, Notification::TYPE = Notification::TYPE::SYSTEM, const TgBot::GenericReply::Ptr& = nullptr) const noexcept;

    void announcing(std::stop_token);

    static TgBot::ReplyKeyboardMarkup::Ptr create_reply(const std::vector<std::vector<std::string>>&);
    static TgBot::InlineKeyboardMarkup::Ptr create_inline(const std::vector<std::vector<std::pair<std::string, std::string>>>&);

private:
    void vps_handler(const TgBot::CallbackQuery::Ptr&);
    void vps_info_editmessage(const TgBot::CallbackQuery::Ptr&, const VPS::Ptr&, PAGE, std::string = std::string());

};
