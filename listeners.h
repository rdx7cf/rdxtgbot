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

void anymsg(const TgBot::Message::Ptr&, const TgBot::Bot&, const std::unique_ptr<Database>&);
void noncom(const TgBot::Message::Ptr&, const TgBot::Bot&, const std::unique_ptr<Database>&);
void start(const TgBot::Message::Ptr&, const TgBot::Bot&, const std::unique_ptr<Database>&);

