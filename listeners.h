#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

#include <tgbot/tgbot.h>

#include "logger.h"
#include "database.h"
#include "userextended.h"
#include "botextended.h"

void anymsg(const TgBot::Message::Ptr&, const BotExtended&);
void noncom(const TgBot::Message::Ptr&, const BotExtended&);
void start(const TgBot::Message::Ptr&, const BotExtended&);

