#pragma once

#include <string>
#include <algorithm>
#include <thread>

#include <tgbot/tgbot.h>

#include "database.h"
#include "logger.h"

class BotExtended : public TgBot::Bot
{
public:

    Database::uPtr database_;

    BotExtended(std::string token, const TgBot::HttpClient& httpClient, const std::string& db_path, const std::string& url = "https://api.telegram.org")
        : TgBot::Bot(token, httpClient, url), database_(new Database(db_path)) {}

    void notify_one(const std::int64_t&, const std::string&);
    void notify_all(const std::string&);

};
