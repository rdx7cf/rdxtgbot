#pragma once

#include <string>
#include <tgbot/tgbot.h>
#include "to_filelog.h"

// https://github.com/reo7sp/tgbot-cpp/issues/226 : SOLVED THE PROBLEM THAT OCCURED WHEN A USER SPAMMED THE BOT WITH MESSAGES (CUSTOM TIMEOUT FOR CURL NEEDED)

class MyHttpClient : public TgBot::CurlHttpClient
{

private:
    std::string makeRequest(const TgBot::Url& url, const std::vector<TgBot::HttpReqArg>& args) const override
    {
        // Repeat, until the request is successful.
        while (1)
        {
            try
            {
                return TgBot::CurlHttpClient::makeRequest(url, args);
            }
            catch (std::runtime_error& e)
            {
                to_filelog(std::string(": ERROR : CURL : ") + std::string(e.what(), std::strlen(e.what()) - 1));
            }

            // Wait a moment for the potential network issue to get resolved,
            // and try again.
            //std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
