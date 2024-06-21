#include "userextended.h"

UserExtended::UserExtended(const TgBot::User::Ptr& tgu, bool b, std::int64_t ms) : TgBot::User(*tgu), blocked(b), member_since(ms) {}
