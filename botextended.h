#pragma once

#include <string>
#include <algorithm>
#include <thread>
#include <functional>

#include <tgbot/tgbot.h>

#include "database.h"
#include "sqlfile.h"
#include "logger.h"

class BotExtended : public TgBot::Bot
{
public:
    enum class Task{SYSTEM = -1, ADS, CURRENCY};

    Userbase::Ptr userbase_;
    Notifbase::Ptr notifbase_;

    BotExtended(std::string token, const TgBot::HttpClient& httpClient, const Userbase::Ptr& users, const Notifbase::Ptr& notif , const std::string& url = "https://api.telegram.org")
        : TgBot::Bot(token, httpClient, url), userbase_(users), notifbase_(notif) {}

    void long_polling(std::stop_token);

    void notify_one(std::int64_t, const std::string&) const noexcept;

    void notify_all(const std::string&, Task = Task::SYSTEM) const noexcept;

    void announcing(std::stop_token, Task);
};

// Listeners

void anymsg(const TgBot::Message::Ptr&, const BotExtended&);
void noncom(const TgBot::Message::Ptr&, const BotExtended&);
void start(const TgBot::Message::Ptr&, const BotExtended&);
