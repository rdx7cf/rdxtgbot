#pragma once

#include <thread>
#include <tgbot/tgbot.h>

namespace GLOBAL
{
    inline std::mutex mutex_db = {};
    inline std::mutex mutex_log = {};
}

void thread_long_polling(std::stop_token, TgBot::Bot&, std::vector<TgBot::User::Ptr>&);
void thread_db_syncing(std::stop_token);
