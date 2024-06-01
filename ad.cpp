#include "ad.h"

Ad::Ad(const std::int64_t& i, const std::string& o, const std::string& t, bool a, const std::int64_t& e)
    : id(i), owner(o), text(t), active(a), expiring_on(e) {}
