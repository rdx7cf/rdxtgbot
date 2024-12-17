#include "BotExtended.h"
#include <algorithm>
#include <thread>
#include <functional>
#include <iostream>
#include <chrono>

#include "Logger.h"
#include "BashCommand.h"
#include "Auxiliary.h"
#include "UserExtended.h"
#include "BotAction.h"

BotExtended::BotExtended(std::string token,
                         const TgBot::HttpClient& http_client,
                         const UserTable::Ptr& usertable,
                         const NotificationTable::Ptr& notificationtable,
                         const VPSTable::Ptr& vpstable,
                         const std::string& url,
                         std::chrono::milliseconds latency,
                         std::int64_t master,
                         const std::vector<char> forbidden_chars)
    : TgBot::Bot(token, http_client, url), usertable_(usertable), notificationtable_(notificationtable), vpstable_(vpstable), latency_(latency), master_(master), forbidden_chars_(forbidden_chars)
{
    getEvents().onAnyMessage(
                [this](TgBot::Message::Ptr message)
    {
        UserExtended::Ptr uptr = std::make_shared<UserExtended>(message->from);

        if(usertable_->add(uptr))
            usertable_->update(uptr);

        std::string log_message = std::string(": INFO : BOT : User [") + std::to_string(message->from->id) + "] [" + message->from->firstName + "] has just sent: '" + (message->text.size() > 32 ? "$something_really_big$" : message->text) + "'.";
        Logger::write(log_message);
    });

    getEvents().onNonCommandMessage(
                [this](TgBot::Message::Ptr message)
    {
        try
        {
            std::this_thread::sleep_for(latency_);
            if(getApi().blockedByUser(message->chat->id))
                return;

            auto user = usertable_->getCopyBy([&message](const UserExtended::Ptr& entry){return entry->id == message->from->id;});

            if(user && !user->pending_actions_->isNoActions())
            {
                auto botaction = user->pending_actions_->getAction();

                botaction->inprogress_messages_.push_back(message);

                if(message->text.size() >= 32)
                {
                    getApi().sendMessage(message->chat->id, R"(*The name should be less than 32 characters\!*)", false, 0, BotExtended::createInline({{{"✕ Close", "close"}}}), "MarkdownV2");
                    botaction->deleteMessages();
                }
                else if(std::find_first_of(message->text.begin(), message->text.end(), forbidden_chars_.begin(), forbidden_chars_.end()) != message->text.end())
                {
                    getApi().sendMessage(message->chat->id, R"(*The name contains a forbidden character\!*)", false, 0, BotExtended::createInline({{{"✕ Close", "close"}}}), "MarkdownV2");
                    botaction->deleteMessages();
                }
                else
                {
                    botaction->user_input_ = message->text;
                    botaction->perform();
                }
            }
            else
            {
                getApi().sendMessage(message->chat->id, "They haven't taught me this command yet.", false, 0);
            }

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

            auto uptr = usertable_->getCopyBy([&message](const UserExtended::Ptr& entry) { return entry->id == message->from->id; });
            std::int64_t vps_counter = 0;


            vpstable_->forRange([&uptr, &vps_counter, this](const VPS::Ptr& entry)
            {
                if(entry->owner_ == 0 || uptr->id == master_ || entry->owner_ == uptr->id)
                    ++vps_counter;
            });

            std::tm ms = localtimeTs(uptr->member_since_);
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

__*VPS Control Panel*__
└`/vps_list` — List the VPS available to you\.
    └__*Settings*__
        ├`Update Information` — Update the VPS information\.
        ├`Screenshot` — Take a screenshot of the VPS desktop\.
        └`Rename` — Rename the VPS\.
    └__*Power Management*__
        ├`Stop` — Hard stop the VPS\.
        ├`Start` — Start the VPS\.
        ├`Reboot` — Hard reboot the VPS\.
        ├`Save` — Save the current state of the VPS\.
        ├`Restore` — Restore the saved state of the VPS\.
        ├`Reset` — Reset the current state of the VPS\.
        ├`Resume` — Resume the VPS from suspension\.
        └`Suspend` — Suspend the VPS\.
    └__*Backup*\.\.\.__

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

            auto f = [&buttons, &message, this](const VPS::Ptr& entry)
            {
                if(entry->owner_ == 0 || message->from->id == master_ || entry->owner_ == message->from->id)
                {
                    buttons.push_back({ std::pair<std::string, std::string>(entry->name_, std::to_string(entry->id_)) });
                }
            };

            vpstable_->forRange(f);

            buttons.push_back({{"✕ Close", "close"}});

            if(buttons.size() != 0)
            {
                getApi().sendMessage(
                            message->chat->id,
                            R"(
*Here are the VPS available to you:*
)",
                            false, 0, BotExtended::createInline(buttons), "MarkdownV2");
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
        std::this_thread::sleep_for(latency_);

        if(getApi().blockedByUser(query->message->chat->id))
            return;

        if(query->data == "close")
        {
            getApi().deleteMessage(query->message->chat->id, query->message->messageId);
        }
        else if(query->data == "cancel")
        {
            auto user = usertable_->getCopyBy([&query](const UserExtended::Ptr& entry){return entry->id == query->from->id;});

            if(user)
                user->pending_actions_->cancelActionByMessageId(query->message->messageId);

            getApi().deleteMessage(query->message->chat->id, query->message->messageId);
        }
        else if(query->message->text == "Here are the VPS available to you:")
        {
            auto vps = vpstable_->getCopyBy([&query](const VPS::Ptr& entry) { return std::to_string(entry->id_) == query->data; });

            if(vps)
                vpsInfoEditMessage(query, vps, VPS_PAGE::MAIN);
            else
            {
                getApi().editMessageText(
                            R"(You can't control the VPS\.")",
                            query->message->chat->id,
                            query->message->messageId);
            }
        }
        else if(StringTools::startsWith(query->message->text, "VPS Information"))
        {
            vpsHandler(query);
        }
    });
}

BotExtended::BotExtended(const BotExtended& bot)
    : TgBot::Bot(bot.getToken()), usertable_(bot.usertable_), notificationtable_(bot.notificationtable_), vpstable_(bot.vpstable_), latency_(bot.latency_), master_(bot.master_), forbidden_chars_(bot.forbidden_chars_) {}

TgBot::ReplyKeyboardMarkup::Ptr BotExtended::createReply(const std::vector<std::vector<std::string>>& layout)
{

    using vecsize = std::vector<std::vector<std::string>>::size_type;

    auto result = std::make_shared<TgBot::ReplyKeyboardMarkup>();
    auto rows = layout.size();

    for(vecsize i = 0; i < rows; ++i)
    {
        std::vector<TgBot::KeyboardButton::Ptr> row;

        for(vecsize j = 0; j < layout[i].size(); ++j)
        {
            auto button = std::make_shared<TgBot::KeyboardButton>();
            button->text = layout[i][j];
            row.push_back(button);
        }

        result->keyboard.push_back(row);
    }

    return result;
}

TgBot::InlineKeyboardMarkup::Ptr BotExtended::createInline(const std::vector<std::vector<std::pair<std::string, std::string>>>& layout)
{

    using vecsize = std::vector<std::vector<std::pair<std::string, std::string>>>::size_type;

    auto result = std::make_shared<TgBot::InlineKeyboardMarkup>();
    auto rows = layout.size();

    for(vecsize i = 0; i < rows; ++i)
    {
        std::vector<TgBot::InlineKeyboardButton::Ptr> row;

        for(vecsize j = 0; j < layout[i].size(); ++j)
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

void BotExtended::vpsHandler(const TgBot::CallbackQuery::Ptr& query)
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
        auto page = static_cast<VPS_PAGE>(std::stoi(query_splitted[1]));
        auto action = std::stoi(query_splitted[2]);

        auto vps = vpstable_->getCopyBy([&id, &query](const VPS::Ptr& entry) {
            return entry->id_ == id;
        });

        if(vps)
        {
            if(page == VPS_PAGE::MAIN)
            {
                page = static_cast<VPS_PAGE>(action);

                /*if(page == VPS_PAGE::MANAGE) // Temporary (or permanent solution for action buttons on the main page).
                    vps->perform(VPS::ACTION::INFO);*/
            }
            else
                vpsProcedure(query, vps, static_cast<VPS::ACTION>(action));

            vpsInfoEditMessage(query, vps, page);
            vpstable_->update(vps);
        }
        else
        {
            getApi().editMessageText(
                        R"(You can't control the VPS.)",
                        query->message->chat->id,
                        query->message->messageId);
        }


    }
    catch (const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::vpsProcedure(const TgBot::CallbackQuery::Ptr& query, const VPS::Ptr& vps, VPS::ACTION a)
{
    if(a == VPS::ACTION::RENAME)
    {
        auto vpsbotaction = std::make_shared<VPSBotAction>(query->message, this, "", vps, a);
        auto user = usertable_->getCopyBy([&query](const UserExtended::Ptr& entry){return entry->id == query->from->id;});

        user->pending_actions_->addAction(vpsbotaction);

        vpsbotaction->inprogress_messages_.push_back(getApi().sendMessage(
                    query->message->chat->id,
R"(*Current VPS name*: `)" + vps->name_ + R"(`
Send me a new name for the specified VPS\.
The name should not contain more than 32 characters or the following __forbidden characters__:
*\_ \* \[ \] \( \) \~ \` \> \# \+ \- \= \| \{ \} \. \! ' "*)",
                    false, 0, BotExtended::createInline({{{"✕ Cancel", "cancel"}}}), "MarkdownV2"));
    }
    else
        vps->perform(a);

    if(a == VPS::ACTION::SCREENSHOT)
    {
        if(boost::filesystem::exists(vps->screenshot_))
        {
            std::tm ms = localtimeTs(boost::filesystem::last_write_time(vps->screenshot_));
            std::ostringstream date;
            date << std::put_time(&ms, "%d/%m/%Y %H:%M:%S");
            getApi().sendPhoto(query->message->chat->id, TgBot::InputFile::fromFile(vps->screenshot_, "image/png"),
R"(*Name*: `)" + vps->name_ + R"(`
*Date*: __)" + date.str() + R"(__)", 0, BotExtended::createInline({{{"✕ Close", "close"}}}), "MarkdownV2");
        }

    }

}

void BotExtended::vpsInfoEditMessage(const TgBot::CallbackQuery::Ptr& query, const VPS::Ptr& vps, VPS_PAGE page, std::string text) const
{
    if(text.size() == 0)
    {
        std::vector<
                std::pair<std::string, std::vector<std::string>>> vec =
        {
            {
                {
                    "*__VPS Information__*", {}
                },
                {
                    "■ *Name*: `" + vps->name_ + "`", {}
                },
                {
                    "■ *UUID*: `" + vps->uuid_ + "`", {}
                },
                {
                    "■ *State*: __" + VPS::string_state(vps->state_) + "__", {}
                },
                {
                    "■ *Threads*: " + vps->cpu_count_, {}
                },
                {
                    "■ *RAM*: " + vps->ram_, {}
                },
                {
                    "■ *Storages*: ",
                    vps->blocks_
                },
                {
                    "■ *Network*: ",
                    vps->netifstat_
                },
                {
                    " ", {}
                },
                {
                    "*__Connection Credentials__*", {}
                },
                {
                    "■ *Address*: `" + vps->address_ + "`", {}
                },
                {
                    "■ *Login*: `" + vps->login_ + "`", {}
                },
                {
                    "■ *Password*: `" + vps->password_ + "`", {}
                }
            }
        };

        text = AUX::generateMessage(vec);
    }



    TgBot::InlineKeyboardMarkup::Ptr buttons;

    switch(page)
    {

    case VPS_PAGE::MAIN:
        buttons = BotExtended::createInline({
                    {{"⛭ Settings", query->data + ":-1:0"}},
                    {{"⭘  Power Management", query->data + ":-1:1"}, {"♺ Backup (Unavailable)", query->data + ":-1:2"}},
                    {{"✕ Close", "close"}}
                    });
        break;

    case VPS_PAGE::MANAGE:
        buttons = BotExtended::createInline({
                    {{"⟳ Update Information", query->data + ":0:0"}},
                    {{"⎙  Screenshot", query->data + ":0:1"}, {"✎ Rename", query->data + ":0:2"}},
                    {{"⇦ Back", query->data + ":-1:-1"}}
                    });
        break;

    case VPS_PAGE::POWER:
        buttons = BotExtended::createInline({
                    {{"Stop", query->data + ":1:10"}, {"Start", query->data + ":1:11"}, {"Reboot", query->data + ":1:12"}},
                    {{"Save", query->data + ":1:13"}, {"Restore", query->data + ":1:14"}, {"Reset", query->data + ":1:15"}},
                    {{"Resume", query->data + ":1:16"}, {"Suspend", query->data + ":1:17"}},
                    {{"⇦ Back", query->data + ":-1:-1"}}
                    });
        break;
    case VPS_PAGE::BACKUP:
        buttons = BotExtended::createInline({
                    {{"⛭ Settings", query->data + ":-1:0"}},
                    {{"⭘  Power Management", query->data + ":-1:1"}, {"♺ Backup (Unavailable)", query->data + ":-1:2"}},
                    {{"✕ Close", "close"}}
                    });
        break;
    }

    try
    {
        getApi().editMessageText(
                        text +
    R"(
*Last action result:* )" + vps->last_output_,
                    query->message->chat->id,
                    query->message->messageId,
                    "",
                    "MarkdownV2",
                    false,
                    buttons
                    );
    }
    catch (const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::vpsInfoEditMessage(const TgBot::Message::Ptr& message, const VPS::Ptr& vps, const TgBot::InlineKeyboardMarkup::Ptr& markup, std::string text) const
{
    if(text.size() == 0)
    {
        std::vector<
                std::pair<std::string, std::vector<std::string>>> vec =
        {
            {
                {
                    "*__VPS Information__*", {}
                },
                {
                    "■ *Name*: `" + vps->name_ + "`", {}
                },
                {
                    "■ *UUID*: `" + vps->uuid_ + "`", {}
                },
                {
                    "■ *State*: __" + VPS::string_state(vps->state_) + "__", {}
                },
                {
                    "■ *Threads*: " + vps->cpu_count_, {}
                },
                {
                    "■ *RAM*: " + vps->ram_, {}
                },
                {
                    "■ *Storages*: ",
                    vps->blocks_
                },
                {
                    "■ *Network*: ",
                    vps->netifstat_
                },
                {
                    " ", {}
                },
                {
                    "*__Connection Credentials__*", {}
                },
                {
                    "■ *Address*: `" + vps->address_ + "`", {}
                },
                {
                    "■ *Login*: `" + vps->login_ + "`", {}
                },
                {
                    "■ *Password*: `" + vps->password_ + "`", {}
                }
            }
        };

        text = AUX::generateMessage(vec);
    }

    try
    {
        getApi().editMessageText(
                        text +
    R"(
*Last action result:* )" + vps->last_output_,
                    message->chat->id,
                    message->messageId,
                    "",
                    "MarkdownV2",
                    false,
                    markup
                    );
    }
    catch (const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::longPolling(std::stop_token tok)
{
    Logger::write(": INFO : BOT : Long polling has been initialized.");

    notifyAll("I'm alive!");

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

void BotExtended::notifyOne(std::int64_t user_id, const std::string& message, const TgBot::GenericReply::Ptr& keyboard, const std::string& parse_mode) const noexcept
{
    try
    {
        getApi().sendMessage(user_id, message, false, 0, keyboard, parse_mode);
        Logger::write(": INFO : BOT : User [" + std::to_string(user_id) + "] has received the message.");
    }
    catch(const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::notifyAll(const std::string& message, Notification::TYPE flag, const TgBot::GenericReply::Ptr& keyboard, const std::string& parse_mode) const noexcept
{
    Logger::write(": INFO : BOT : Notifying all users...");

    auto f = [this, &message, &flag, &keyboard, &parse_mode](UserExtended::Ptr& user)
    {
        try
        {
            if(!getApi().blockedByUser(user->id))
            {
                if(flag == Notification::TYPE::SYSTEM || user->active_tasks_[static_cast<int>(flag)])
                        notifyOne(user->id, message, keyboard, (parse_mode == "Not assigned" ? "" : parse_mode));
            }
            else
                Logger::write(": INFO : BOT : User [" + std::to_string(user->id) + "] blocked the bot.");
        }
        catch(const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    };
    usertable_->forRange(f);

    Logger::write(": INFO : BOT : Users have been notified.");
}

void BotExtended::announcing(std::stop_token tok) const
{
    std::time_t current_timestamp;
    std::tm current;

    auto f = [this, &current, &current_timestamp](Notification::Ptr& notif)
    {
        std::for_each(notif->schedule_.begin(), notif->schedule_.end(), [this, &current, &current_timestamp, &notif](TmExtended& time_point)
        {
            if(current.tm_wday == time_point.tm_wday && notif->active_)
            {
                if(current_timestamp >= notif->expiring_on_)
                {
                    notif->active_ = false;
                    return;
                }
                if (((current.tm_hour == time_point.tm_hour && current.tm_min >= time_point.tm_min) || current.tm_hour > time_point.tm_hour) && !time_point.executed_) // Такое монструозное условие нужно для того, чтобы учитывалась разница и между часами, и между часами:минутами (то есть чтобы временная точка типа 15:30 также была допустима)
                {
                    notifyAll(notif->text_, notif->type_, nullptr, notif->parse_mode_);
                    time_point.executed_ = true;
                }
                else if((current.tm_hour < time_point.tm_hour || (current.tm_hour == time_point.tm_hour && current.tm_min < time_point.tm_min)) && time_point.executed_)
                {
                    time_point.executed_ = false;
                }
            }
        });
    };

    while(!tok.stop_requested())
    {
        current_timestamp = std::time(nullptr);
        current = localtimeTs(current_timestamp);

        notificationtable_->forRange(f);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


