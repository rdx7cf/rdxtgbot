#include "ad.h"

Ad::Ad(std::int64_t i, const std::string& o, const std::string& t, bool a, std::time_t ad, std::time_t e)
    : id(i), owner(o), text(t), active(a), added_on(ad), expiring_on(e) {}
