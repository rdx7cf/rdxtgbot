#include "botextended.h"

BotExtended::BotExtended(std::string token, const TgBot::HttpClient& httpClient, const Userbase::Ptr& userbase, const Notifbase::Ptr& notifbase, const VPSbase::Ptr& vpsbase, const std::string& url)
    : TgBot::Bot(token, httpClient, url), userbase_(userbase), notifbase_(notifbase), vpsbase_(vpsbase)
{
    getEvents().onAnyMessage(
                [this](TgBot::Message::Ptr message)
    {
        UserExtended::Ptr uptr = std::make_shared<UserExtended>(message->from);

        if(userbase_->add(uptr))
            userbase_->update(uptr);

        std::string log_message = std::string(": INFO : BOT : User [") + std::to_string(message->from->id) + "] [" + message->from->firstName + "] has just sent: '" + message->text + "'.";
        Logger::write(log_message);
    });

    getEvents().onNonCommandMessage(
                [this](TgBot::Message::Ptr message)
    {
        try
        {
            if(getApi().blockedByUser(message->chat->id))
                return;

            getApi().sendMessage(message->chat->id, "They haven't taught me this command yet.", false, 0);
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    getEvents().onCommand("start",
                [this](TgBot::Message::Ptr message)
    {
        try
        {
            if(getApi().blockedByUser(message->chat->id))
                return;

            auto uptr = userbase_->get_copy_by([&message](const UserExtended::Ptr& entry) { return entry->id == message->from->id; });
            std::int64_t vps_counter = 0;


            vpsbase_->for_range([&uptr, &vps_counter](const VPS::Ptr& entry)
            {
                if(entry->owner == 0 || uptr->id == MASTER || entry->owner == uptr->id)
                    ++vps_counter;
            });

            std::tm ms = localtime_ts(uptr->member_since);
            std::ostringstream oss;
            oss << std::put_time(&ms, "%d/%m/%Y %H:%M:%S");

            getApi().sendMessage(
                        message->chat->id,
                        std::string(R"(
*Your account details*
├Member Since: ||__)") + oss.str() + R"(__||
├Telegram ID: `)" + std::to_string(uptr->id) + R"(`
└Available VPS: *)" + std::to_string(vps_counter) + R"(*

Type `/info` for help\.
                        )",
                        false, 0, nullptr, "MarkdownV2");
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    getEvents().onCommand("info",
                [this](TgBot::Message::Ptr message)
    {
        try
        {
            if(getApi().blockedByUser(message->chat->id))
                return;

            getApi().sendMessage(
                        message->chat->id,
                        R"(
They've finally taught me something\. Take a look at what I'm able to do for you now\.

🖥️ *VPS Control Panel*
└`/vps_list` — List the VPS available to you\.
    ├*Update Information* — Update the VPS information\.
    ├*Stop* — Hard stop the VPS\.
    ├*Start* — Start the VPS\.
    ├*Reboot* — Hard reboot the VPS\.
    ├*Save* — Save the current state of the VPS\.
    ├*Restore* — Restore the saved state of the VPS\.
    ├*Reset* — Reset the current state of the VPS\.
    ├*Resume* — Resume the VPS from suspension\.
    └*Suspend* — Suspend the VPS\.

Got any questions? Ask them [here](tg://user?id=1373205351)\.
                        )",
                        false, 0, nullptr, "MarkdownV2");
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    getEvents().onCommand("vps_list",
                [this](TgBot::Message::Ptr message)
    {
        try
        {
            if(getApi().blockedByUser(message->chat->id))
                return;

            std::vector<std::vector<std::pair<std::string, std::string>>> buttons;

            auto f = [&buttons, &message](const VPS::Ptr& entry)
            {
                if(entry->owner == 0 || message->from->id == MASTER || entry->owner == message->from->id)
                {
                    buttons.push_back({ std::pair<std::string, std::string>(entry->name, std::to_string(entry->id)) });
                }
            };

            vpsbase_->for_range(f);

            buttons.push_back({{"✕ Close", "close"}});

            if(buttons.size() != 0)
            {
                getApi().sendMessage(
                            message->chat->id,
                            R"(
*Here are the VPS available to you:*
)",
                            false, 0, BotExtended::create_inline(buttons), "MarkdownV2");
            }
            else
            {
                getApi().sendMessage(message->chat->id, R"(You have no VPS available.)");
            }


        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    getEvents().onCallbackQuery(
                [this](TgBot::CallbackQuery::Ptr query)
    {
        if(query->data == "close")
        {
            getApi().deleteMessage(query->message->chat->id, query->message->messageId);
        }
        else if(query->message->text == "Here are the VPS available to you:")
        {
            auto vps = vpsbase_->get_copy_by([&query](const VPS::Ptr& entry) { return std::to_string(entry->id) == query->data; });

            if(vps)
                vps_info_editmessage(query, vps, PAGE::MAIN);
            else
            {
                getApi().editMessageText(
                            R"(You can't control ")" + vps->name + R"(".)",
                            query->message->chat->id,
                            query->message->messageId);
            }
        }
        else if(StringTools::startsWith(query->message->text, "VPS Information"))
        {
            vps_handler(query);
        }
    });
}

TgBot::ReplyKeyboardMarkup::Ptr BotExtended::create_reply(const std::vector<std::vector<std::string>>& layout)
{

    using vecsize = std::vector<std::vector<std::string>>::size_type;

    auto result = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto rows = layout.size();
    auto cols = layout[0].size();

    for(vecsize i = 0; i < rows; ++i)
    {
        std::vector<TgBot::KeyboardButton::Ptr> row;

        for(vecsize j = 0; j < cols; ++j)
        {
            auto button = std::make_shared<TgBot::KeyboardButton>();
            button->text = layout[i][j];
            row.push_back(button);
        }

        result->keyboard.push_back(row);
    }

    return result;
}

TgBot::InlineKeyboardMarkup::Ptr BotExtended::create_inline(const std::vector<std::vector<std::pair<std::string, std::string>>>& layout)
{

    using vecsize = std::vector<std::vector<std::pair<std::string, std::string>>>::size_type;

    auto result = std::make_shared<TgBot::InlineKeyboardMarkup>();
    auto rows = layout.size();
    auto cols = layout[0].size();

    for(vecsize i = 0; i < rows; ++i)
    {
        std::vector<TgBot::InlineKeyboardButton::Ptr> row;

        for(vecsize j = 0; j < cols; ++j)
        {
            auto button = std::make_shared<TgBot::InlineKeyboardButton>();
            button->text = layout[i][j].first;
            button->callbackData = layout[i][j].second;
            row.push_back(button);
        }

        result->inlineKeyboard.push_back(row);
    }

    return result;
}

void BotExtended::vps_handler(const TgBot::CallbackQuery::Ptr& query)
{
    try
    {
        auto query_splitted = StringTools::split(query->data, ':');

        if(query_splitted.size() != 3)
        {
            getApi().deleteMessage(query->message->chat->id, query->message->messageId);
            return;
        }

        query->data = query_splitted[0];

        auto id = std::stoi(query_splitted[0]);
        auto page = static_cast<PAGE>(std::stoi(query_splitted[1]));
        auto action = std::stoi(query_splitted[2]);

        auto vps = vpsbase_->get_copy_by([&id](const VPS::Ptr& entry) {
            return entry->id == id;
        });

        if(vps)
        {
            if(page == PAGE::MAIN)
            {
                page = static_cast<PAGE>(action);

                if(page == PAGE::MANAGE) // Temporary (or permanent solution for action buttons on the main page).
                    vps->perform(VPS::ACTION::INFO);
            }
            else
                vps->perform(static_cast<VPS::ACTION>(action));

            vps_info_editmessage(query, vps, page);
            vpsbase_->update(vps);
        }
        else
        {
            getApi().editMessageText(
                        R"(You can't control ")" + vps->name + R"(".)",
                        query->message->chat->id,
                        query->message->messageId);
        }


    }
    catch (const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::vps_info_editmessage(const TgBot::CallbackQuery::Ptr& query, const VPS::Ptr& vps, PAGE page, std::string message)
{
    if(!message.size())
    {
        message =
R"(*VPS Information*
├*Name*: `)" + vps->name + R"(`
├*UUID*: `)" + vps->uuid + R"(`
├*State*: __)" + VPS::string_state(vps->state) + R"(__
├*Threads*: )" + vps->cpu_count + R"(
├*RAM*: )" + vps->ram + R"(
└*Storages*:)";

        auto border = vps->blocks.size() - 1;

        for(VPS::blockvec::size_type i = 0; i < border; ++i)
        {
            message +=
R"(
    ├*)" + vps->blocks[i].first + "*: " + vps->blocks[i].second + ";";
        }

        message +=
R"(
    └*)" + vps->blocks[border].first + "*: " + vps->blocks[border].second + "\\.\n";
    }


    TgBot::InlineKeyboardMarkup::Ptr buttons;

    switch(page)
    {
    case PAGE::MAIN:
    case PAGE::MANAGE:
        buttons = BotExtended::create_inline({
                    {{"⟳ Update Information", query->data + ":-1:0"}},
                    {{"⏻  Power Management", query->data + ":-1:1"}, {"♺ Backup (Unavailable)", query->data + ":-1:2"}},
                    {{"✕ Close", "close"}}
                    });
        break;

    case PAGE::POWER:
        buttons = BotExtended::create_inline({
                    {{"Stop", query->data + ":1:10"}, {"Start", query->data + ":1:11"}, {"Reboot", query->data + ":1:12"}},
                    {{"Save", query->data + ":1:13"}, {"Restore", query->data + ":1:14"}, {"Reset", query->data + ":1:15"}},
                    {{"Resume", query->data + ":1:16"}, {"Suspend", query->data + ":1:17"}},
                    {{"⇦ Back", query->data + ":-1:-1"}}
                    });
        break;
    case PAGE::BACKUP:
        break;
    }

    getApi().editMessageText(
                    message +
R"(
*Last action result:* )" + vps->last_output,
                query->message->chat->id,
                query->message->messageId,
                "",
                "MarkdownV2",
                false,
                buttons
                );
}

void BotExtended::long_polling(std::stop_token tok)
{
    Logger::write(": INFO : BOT : Long polling has been initialized.");

    notify_all("I'm alive!");

    TgBot::TgLongPoll longPoll(*this, 100, 1);

    while(!tok.stop_requested())
    {
        try
        {
            longPoll.start();
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    }
    Logger::write(": INFO : BOT : Long polling has been stopped.");

}

void BotExtended::notify_one(std::int64_t user_id, const std::string& message, const TgBot::GenericReply::Ptr& keyboard) const noexcept
{
    try
    {
        getApi().sendMessage(user_id, message, false, 0, keyboard);
        Logger::write(": INFO : BOT : User [" + std::to_string(user_id) + "] has received the message.");
    }
    catch(const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::notify_all(const std::string& message, Notification::TYPE flag, const TgBot::GenericReply::Ptr& keyboard) const noexcept
{
    Logger::write(": INFO : BOT : Notifying all users...");

    auto f = [this, &message, &flag, &keyboard](UserExtended::Ptr& user)
    {
        try
        {
            if(!getApi().blockedByUser(user->id))
            {
                if(flag == Notification::TYPE::SYSTEM || user->activeTasks[static_cast<int>(flag)])
                        notify_one(user->id, message, keyboard);
            }
            else
                Logger::write(": INFO : BOT : User [" + std::to_string(user->id) + "] blocked the bot.");
        }
        catch(const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    };
    userbase_->for_range(f);

    Logger::write(": INFO : BOT : Users has been notified.");
}

void BotExtended::announcing(std::stop_token tok)
{
    std::time_t current_timestamp;
    std::tm current;

    auto f = [this, &current, &current_timestamp](Notification::Ptr& notif)
    {
        std::for_each(notif->schedule.begin(), notif->schedule.end(), [this, &current, &current_timestamp, &notif](TmExtended& time_point)
        {
            if(current.tm_wday == time_point.tm_wday && notif->active)
            {
                if(current_timestamp >= notif->expiring_on)
                {
                    notif->active = false;
                    return;
                }
                if (((current.tm_hour == time_point.tm_hour && current.tm_min >= time_point.tm_min) || current.tm_hour > time_point.tm_hour) && !time_point.executed) // Такое монструозное условие нужно для того, чтобы учитывалась разница и между часами, и между часами:минутами (то есть чтобы временная точка типа 15:30 также была допустима)
                {
                    notify_all(notif->text, notif->type);
                    time_point.executed = true;
                }
                else if((current.tm_hour < time_point.tm_hour || (current.tm_hour == time_point.tm_hour && current.tm_min < time_point.tm_min)) && time_point.executed)
                {
                    time_point.executed = false;
                }
            }
        });
    };

    while(!tok.stop_requested())
    {
        current_timestamp = std::time(nullptr);
        current = localtime_ts(current_timestamp);

        notifbase_->for_range(f);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


