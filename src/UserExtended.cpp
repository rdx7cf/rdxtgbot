#include "UserExtended.h"

bool UserExtended::operator==(const UserExtended& rhs) const
{
    if(id != rhs.id)
        return false;

    return true;
}

/*bool UserExtended::updateNeeded(const UserExtended& rhs) const
{
    if(username != rhs.username)
        return true;
    if(firstName != rhs.firstName)
        return true;
    if(lastName != rhs.lastName)
        return true;
    if(languageCode != rhs.languageCode)
        return true;
    if(isBot != rhs.isBot)
        return true;
    if(isPremium != rhs.isPremium)
        return true;
    if(addedToAttachmentMenu != rhs.addedToAttachmentMenu)
        return true;
    if(canJoinGroups != rhs.canJoinGroups)
        return true;
    if(canReadAllGroupMessages != rhs.canReadAllGroupMessages)
        return true;
    if(supportsInlineQueries != rhs.supportsInlineQueries)
        return true;
    if(active_tasks_ != rhs.active_tasks_)
        return true;

    return false;
}

UserExtended& UserExtended::operator=(const UserExtended& rhs)
{
    if(this == &rhs)
        return *this;

    id = rhs.id;
    username = rhs.username;
    firstName = rhs.firstName;
    lastName = rhs.lastName;
    languageCode = rhs.languageCode;
    isBot = rhs.isBot;
    isPremium = rhs.isPremium;
    addedToAttachmentMenu = rhs.addedToAttachmentMenu;
    canJoinGroups = rhs.canJoinGroups;
    canReadAllGroupMessages = rhs.canReadAllGroupMessages;
    supportsInlineQueries = rhs.supportsInlineQueries;
    active_tasks_ = rhs.active_tasks_;
    member_since_ = rhs.member_since_;

    return *this;
}*/
