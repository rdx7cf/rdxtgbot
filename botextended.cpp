#include "botextended.h"

void BotExtended::long_polling(std::stop_token tok)
{
    TgBot::InlineKeyboardMarkup::Ptr kb_initial = std::make_shared<TgBot::InlineKeyboardMarkup>();

    std::vector<TgBot::InlineKeyboardButton::Ptr> row0;

    TgBot::InlineKeyboardButton::Ptr list_servers = std::make_shared<TgBot::InlineKeyboardButton>();
    list_servers->text = "List Servers";
    list_servers->callbackData = "list_servers";

    row0.push_back(list_servers);
    kb_initial->inlineKeyboard.push_back(row0);

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
                [this, &kb_initial](TgBot::Message::Ptr message)
    {
        try
        {
            if(getApi().blockedByUser(message->chat->id))
                return;

            getApi().sendMessage(message->chat->id, "They haven't taught me this command yet.", false, 0, kb_initial);
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    getEvents().onCommand("start",
                [this, &kb_initial](TgBot::Message::Ptr message)
    {
        try
        {
            if(getApi().blockedByUser(message->chat->id))
                return;

            getApi().sendMessage(message->chat->id, "They've finally taught me something. Take a look at what I'm able to do for you now.", false, 0, kb_initial);
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }
    });

    getEvents().onCallbackQuery(
            [this, &kb_initial](TgBot::CallbackQuery::Ptr query)
    {
        try
        {
            auto user = userbase_->get_copy_by_id(query->from->id);

            if(query->data == "list_servers")
            {
                if(user->vps_names.size() == 0)
                    getApi().editMessageText("You have no VPS available. Maybe I can help you another way?", query->from->id, query->message->messageId, std::string(), std::string(), false, kb_initial);
                else
                {
                    TgBot::InlineKeyboardMarkup::Ptr kb_servers = std::make_shared<TgBot::InlineKeyboardMarkup>();

                    for(const auto& vps_name : user->vps_names)
                    {
                        std::vector<TgBot::InlineKeyboardButton::Ptr> row;

                        TgBot::InlineKeyboardButton::Ptr vps_button = std::make_shared<TgBot::InlineKeyboardButton>();
                        vps_button->text = vps_name;
                        vps_button->callbackData = std::string("v:") + vps_name;

                        row.push_back(vps_button);
                        kb_servers->inlineKeyboard.push_back(row);
                    }

                    getApi().editMessageText("Choose a VPS to operate with.", query->from->id, query->message->messageId, std::string(), std::string(), false, kb_servers);
                }
            }
            else if(StringTools::startsWith(query->data, "v:"))
            {
                TgBot::InlineKeyboardMarkup::Ptr kb_actions = std::make_shared<TgBot::InlineKeyboardMarkup>();
                std::vector<TgBot::InlineKeyboardButton::Ptr> row;

                TgBot::InlineKeyboardButton::Ptr reboot_button = std::make_shared<TgBot::InlineKeyboardButton>();
                reboot_button->text = "Reboot";
                reboot_button->callbackData = std::string("a:reboot:") + query->data;
                row.push_back(reboot_button);

                TgBot::InlineKeyboardButton::Ptr stop_button = std::make_shared<TgBot::InlineKeyboardButton>();
                stop_button->text = "Stop";
                stop_button->callbackData = std::string("a:stop:") + query->data;
                row.push_back(stop_button);

                TgBot::InlineKeyboardButton::Ptr start_button = std::make_shared<TgBot::InlineKeyboardButton>();
                start_button->text = "Start";
                start_button->callbackData = std::string("a:start:") + query->data;
                row.push_back(start_button);

                kb_actions->inlineKeyboard.push_back(row);

                getApi().editMessageText("Choose an action to perform.", query->from->id, query->message->messageId, std::string(), std::string(), false, kb_actions);

            }
            else if(StringTools::startsWith(query->data, "a:"))
            {
                auto user = userbase_->get_copy_by_id(query->from->id);
                std::vector<std::string> input = StringTools::split(query->data, ':');

                if(std::search(user->vps_names_str.begin(), user->vps_names_str.end(), input[3].begin(), input[3].end()) == user->vps_names_str.end())
                {
                    getApi().editMessageText("You're not allowed to perform this action.", query->from->id, query->message->messageId);
                    getApi().sendMessage(query->from->id, "How can I help you?", false, 0, kb_initial);
                    return;
                }

                BashCommand cmd;
                if(input[1] == "reboot")
                {
                    cmd.Command = std::string("virsh destroy ") + input[3] + " && virsh start " + input[3];
                    cmd.execute();

                    if(!cmd.ExitStatus)
                        getApi().editMessageText(std::string("The VPS \"") + input[3] + "\" has been restarted. Well, at least it didn't crashed me.\n"
                                                            "Here's the raw output:\n\nstderr:\n<code>" + cmd.StdErr + "</code>\n\nstdout:\n<code>" + cmd.StdOut + "</code>",
                                                 query->from->id,
                                                 query->message->messageId, std::string(), "HTML");
                    else
                        getApi().editMessageText(std::string("Something went wrong while restarting \"") + input[3] + ".\n"
                                                             "Here's the raw output:\n\nstderr:\n<code>" + cmd.StdErr + "</code>\n\nstdout:\n<code>" + cmd.StdOut + "</code>",
                                                 query->from->id,
                                                 query->message->messageId, std::string(), "HTML");
                }
                else if(input[1] == "stop")
                {
                    cmd.Command = std::string("virsh destroy ") + input[3];
                    cmd.execute();

                    if(!cmd.ExitStatus)
                        getApi().editMessageText(std::string("The VPS \"") + input[3] + "\" has been stoped. Well, at least it didn't crashed me.\n"
                                                 "Here's the raw output:\n\nstderr:\n<code>" + cmd.StdErr + "</code>\n\nstdout:\n<code>" + cmd.StdOut + "</code>",
                                                 query->from->id,
                                                 query->message->messageId, std::string(), "HTML");
                    else
                        getApi().editMessageText(std::string("Something went wrong while stoping the VPS \"") + input[3] + "\".\n"
                                                "Here's the raw output:\n\nstderr:\n<code>" + cmd.StdErr + "</code>\n\nstdout:\n<code>" + cmd.StdOut + "</code>",
                                                 query->from->id,
                                                 query->message->messageId, std::string(), "HTML");
                }
                else if(input[1] == "start")
                {
                    cmd.Command = std::string("virsh start ") + input[3];
                    cmd.execute();

                    if(!cmd.ExitStatus)
                        getApi().editMessageText( std::string("The VPS \"") + input[3] + "\" has been started. Well, at least it didn't crashed me.\n"
                                                                                         "Here's the raw output:\n\nstderr:\n<code>" + cmd.StdErr + "</code>\n\nstdout:\n<code>" + cmd.StdOut + "</code>",
                                                  query->from->id,
                                                  query->message->messageId, std::string(), "HTML");
                    else
                        getApi().editMessageText(std::string("Something went wrong while starting the VPS \"") + input[3] + "\".\n"
                                                 "Here's the raw output:\n\nstderr:\n<code>" + cmd.StdErr + "</code>\n\nstdout:\n<code>" + cmd.StdOut + "</code>",
                                                 query->from->id,
                                                 query->message->messageId, std::string(), "HTML");
                }

                getApi().sendMessage(query->from->id, "How can I help you?", false, 0, kb_initial);
            }
        }
        catch (const std::exception& e)
        {
            Logger::write(std::string(": ERROR : BOT : ") + e.what() + ".");
        }

    });

    Logger::write(": INFO : BOT : Long polling has been initialized.");

    notify_all("I'm alive! If you need me, poke me with one of the buttons below.", Task::SYSTEM, kb_initial);

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


