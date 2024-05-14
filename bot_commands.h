#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <tgbot/tgbot.h>

#include "to_filelog.h"
#include "multithreading.h"

#include "database.h"

void anymsg(const TgBot::Message::Ptr&, const TgBot::Bot&, const std::unique_ptr<Database>&);
void start(const TgBot::Message::Ptr&, const TgBot::Bot&, const std::unique_ptr<Database>&);

