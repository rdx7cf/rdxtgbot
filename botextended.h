#pragma once

#include <string>
#include <algorithm>
#include <thread>
#include <functional>

#include <tgbot/tgbot.h>

#include "database.h"
#include "sqlfile.h"
#include "logger.h"
#include "bashcommand.h"
#include "userextended.h"

class BotExtended : public TgBot::Bot
{
public:
    enum class Task{SYSTEM = -1, ADS, CURRENCY};

    Userbase::Ptr userbase_;
    Notifbase::Ptr notifbase_;
    VPSbase::Ptr vpsbase_;

    BotExtended(std::string token, const TgBot::HttpClient& httpClient, const Userbase::Ptr& userbase, const Notifbase::Ptr& notifbase, const VPSbase::Ptr& vpsbase, const std::string& url = "https://api.telegram.org")
        : TgBot::Bot(token, httpClient, url), userbase_(userbase), notifbase_(notifbase), vpsbase_(vpsbase) {}

    void long_polling(std::stop_token);

    void notify_one(std::int64_t, const std::string&, const TgBot::GenericReply::Ptr& = nullptr) const noexcept;

    void notify_all(const std::string&, Task = Task::SYSTEM, const TgBot::GenericReply::Ptr& = nullptr) const noexcept;

    void announcing(std::stop_token, Task);

private:
    void vps_action_handler(const TgBot::Message::Ptr&, VPS::ACTION, std::string::size_type);
};
