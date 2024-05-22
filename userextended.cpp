#include "userextended.h"

UserExtended::UserExtended() {}

UserExtended::UserExtended(const TgBot::User::Ptr& tgu) : TgBot::User(*tgu) {}
