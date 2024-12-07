#include "notification.h"

Notification::Notification(std::int64_t i, const std::string& o, const std::string& t, bool a, TYPE ty, std::time_t ad_on, std::time_t ex_on)
    : id(i), owner(o), text(t), active(a), type(ty), added_on(ad_on), expiring_on(ex_on) {}
