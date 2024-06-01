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

    Database<UserExtended>::uPtr userbase_;
    Database<Ad>::uPtr adbase_;

    BotExtended(std::string token, const TgBot::HttpClient& httpClient, const std::string& ub_path, const std::string& ab_path, const std::string& url = "https://api.telegram.org")
        : TgBot::Bot(token, httpClient, url), userbase_(new Database<UserExtended>(ub_path)), adbase_(new Database<Ad>(ab_path)) {}

    void notify_one(const std::int64_t&, const std::string&);
    void notify_all(const std::string&);

};
