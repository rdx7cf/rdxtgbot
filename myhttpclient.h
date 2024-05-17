#pragma once

#include <string>
#include <tgbot/tgbot.h>
#include "logger.h"

// https://github.com/reo7sp/tgbot-cpp/issues/226

class MyHttpClient : public TgBot::CurlHttpClient
{
private:
    std::string makeRequest(const TgBot::Url& url, const std::vector<TgBot::HttpReqArg>& args) const override
    {
        while (1)
        {
            try
            {
                return TgBot::CurlHttpClient::makeRequest(url, args);
            }
            catch (std::runtime_error& e)
            {
                //to_filelog(std::string(": ERROR : CURL : ") + std::string(e.what(), std::strlen(e.what()) - 1));
            }
            //std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
