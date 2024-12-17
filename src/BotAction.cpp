#include "BotAction.h"
#include "BotExtended.h"

using namespace std::literals::chrono_literals;

BotAction::BotAction(const TgBot::Message::Ptr& initial_message, const BotExtended* bot, const std::string& user_input)
    : bot_(bot), user_input_(user_input)
{
    inprogress_messages_.push_back(initial_message);
}

VPSBotAction::VPSBotAction(const TgBot::Message::Ptr& initial_message, const BotExtended* bot, const std::string& user_input, const VPS::Ptr& vps, VPS::ACTION action)
    :  BotAction(initial_message, bot, user_input), vps_(vps), action_(action)
{}

void BotAction::deleteMessages()
{

    std::for_each(inprogress_messages_.begin() + 1, inprogress_messages_.end(), [this](const TgBot::Message::Ptr& msg)
    {
        std::this_thread::sleep_for(bot_->LATENCY);
        try
        {
            bot_->getApi().deleteMessage(msg->chat->id, msg->messageId);
        }
        catch(const std::exception& e)
        { Logger::write(std::string(": ERROR : BOT : ") + e.what() + "."); }
    });

    inprogress_messages_.clear();
}

void VPSBotAction::perform()
{
    vps_->perform(action_, user_input_);
    bot_->vpstable_->update(vps_);

    auto controlmessage = inprogress_messages_[0];
    bot_->vpsInfoEditMessage(controlmessage, vps_, controlmessage->replyMarkup);

    deleteMessages();
}

void BotAction::List::addAction(const BotAction::Ptr& a)
{
    std::lock_guard<std::mutex> lock(mtx_q_);
    pending_actions_.push_back(a);
}

BotAction::Ptr BotAction::List::getAction()
{
    std::lock_guard<std::mutex> lock(mtx_q_);
    auto result = pending_actions_.back();
    pending_actions_.pop_back();

    return result;
}

bool BotAction::List::isNoActions()
{
    std::lock_guard<std::mutex> lock(mtx_q_);
    return pending_actions_.empty();
}

bool BotAction::List::cancelActionByMessageId(std::int32_t messageId)
{
    auto botaction_it = std::find_if(pending_actions_.begin(), pending_actions_.end(),
                                  [&messageId](const BotAction::Ptr& entry)
    {
        auto message_to_delete = std::find_if(entry->inprogress_messages_.begin(), entry->inprogress_messages_.end(), [&messageId](const TgBot::Message::Ptr& msg)
        {
           return messageId == msg->messageId;
        });

        if(message_to_delete != entry->inprogress_messages_.end())
            return true;

        return false;
    });

    if(botaction_it != pending_actions_.end())
    {
        pending_actions_.erase(botaction_it);
        return true;
    }

    return false;
}
