#pragma once

#include <thread>

#include <tgbot/tgbot.h>

#include "botextended.h"
#include "logger.h"
#include "listeners.h"

void thread_long_polling(std::stop_token, BotExtended&);
void thread_auto_sync(std::stop_token, const BotExtended&, const std::int32_t&);
