#include "botextended.h"

static TgBot::ReplyKeyboardMarkup::Ptr create_keyboard(const std::vector<std::vector<std::string>>& layout)
{

    using vecsize = std::vector<std::vector<std::string>>::size_type;

    auto result = std::make_shared<TgBot::ReplyKeyboardMarkup>();

    for(vecsize i = 0; i < layout.size(); ++i)
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


void BotExtended::vps_action_handler(const TgBot::Message::Ptr& message, VPS::ACTION action, int textsize)
{
    try
    {
        if(getApi().blockedByUser(message->chat->id) || message->text.size() < textsize)
            return;

        std::string vps_name(message->text, textsize);

        auto vps = vpsbase_->get_copy_by_owner_n_name(message->from->id, vps_name);

        if(vps)
        {
            getApi().sendMessage(
                        message->chat->id,
                        vps->perform(action),
                        false, 0, nullptr, "MarkdownV2");
        }
        else
        {
            getApi().sendMessage(
                        message->chat->id,
                        R"(You can't control ")" + vps_name + R"(". Is that correct VPS name?)",
                        false, 0, nullptr, "HTML");
        }
    }
    catch (const std::exception& e)
    {
        Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
    }
}

void BotExtended::long_polling(std::stop_token tok)
{

    /*auto kb_initial = create_keyboard({
                                          {"Show Info"}
                                      });*/

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

            getApi().sendMessage(
                        message->chat->id,
                        R"(
They've finally taught me something\. Take a look at what I'm able to do for you now\.

`/vps_list` — Listing the VPS available to you\.
`/vps_reboot $VPS` — Hard rebooting the specificed VPS\.
`/vps_stop $VPS` — Hard stoping the specified VPS\.
`/vps_run $VPS` — Running the specified VPS\.

Got any questions? Ask them [here](tg://user?id=1373205351)\.
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

🖥️*VPS Control Panel*
├`/vps_list` — Listing the VPS available to you\.
├`/vps_info $VPS` — Printing additional information about the specified VPS\.
├`/vps_reboot $VPS` — Hard rebooting the specified VPS\.
├`/vps_suspend $VPS` — Suspending the specified VPS\.
├`/vps_resume $VPS` — Resuming the specified VPS from suspension\.
├`/vps_reset $VPS` — Resetting the current state of the specified VPS\.
├`/vps_save $VPS` — Save the current state of the specified VPS\.
├`/vps_restore $VPS` — Restoring the saved state of the specified VPS\.
├`/vps_stop $VPS` — Hard stoping the specified VPS\.
└`/vps_start $VPS` — Running the specified VPS\.

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

            std::string result;

            auto f = [&message, &result](const VPS::Ptr& entry)
            {
                if(entry->owner == message->from->id)
                    result += ('`' + entry->name + '`') + '\n';
            };

            vpsbase_->for_range(f);

            if(result.size() != 0)
            {
                getApi().sendMessage(
                            message->chat->id,
                            R"(
Here are the VPS available to you:

)" + result + "",
                            false, 0, nullptr, "MarkdownV2");
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

    getEvents().onCommand("vps_info",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::INFO, 10);
    });

    getEvents().onCommand("vps_reboot",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::REBOOT, 12);
    });

    getEvents().onCommand("vps_suspend",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::SUSPEND, 13);
    });

    getEvents().onCommand("vps_resume",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::RESUME, 12);
    });

    getEvents().onCommand("vps_reset",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::RESET, 11);
    });

    getEvents().onCommand("vps_save",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::SAVE, 10);
    });

    getEvents().onCommand("vps_restore",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::RESTORE, 13);
    });


    getEvents().onCommand("vps_stop",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::STOP, 10);
    });

    getEvents().onCommand("vps_start",
                [this](TgBot::Message::Ptr message)
    {
        vps_action_handler(message, VPS::ACTION::START, 11);
    });


    Logger::write(": INFO : BOT : Long polling has been initialized.");

    notify_all("I'm alive! If you need me, poke me with one of the buttons below.", Task::SYSTEM);

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

void BotExtended::notify_all(const std::string& message, Task flag, const TgBot::GenericReply::Ptr& keyboard) const noexcept
{
    Logger::write(": INFO : BOT : Notifying all users...");

    auto f = [this, &message, &flag, &keyboard](UserExtended::Ptr& user)
    {
        try
        {
            if(!getApi().blockedByUser(user->id))
            {
                if(flag == Task::SYSTEM || user->activeTasks[static_cast<int>(flag)])
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

void BotExtended::announcing(std::stop_token tok, Task t)
{
    std::time_t current_timestamp;
    std::tm current;

    auto f = [this, &current, &current_timestamp, &t](Notification::Ptr& notif)
    {
        std::for_each(notif->schedule.begin(), notif->schedule.end(), [this, &current, &current_timestamp, &notif, &t](TmExtended& time_point)
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
                    notify_all(notif->text, t);
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


