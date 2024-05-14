#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <tgbot/tgbot.h>

#include "to_filelog.h"
#include "multithreading.h"

void anymsg(const TgBot::Message::Ptr&, const TgBot::Bot&, std::vector<TgBot::User::Ptr>&);
void start(const TgBot::Message::Ptr&, const TgBot::Bot&, std::vector<TgBot::User::Ptr>&);

