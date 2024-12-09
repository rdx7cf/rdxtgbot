#include "notification.h"

Notification::Notification(std::int64_t i,
                           const std::string& o,
                           const std::string& t,
                           bool a,
                           TYPE ty,
                           const std::string& tp_str,
                           const std::string& wd_str,
                           const std::vector<TmExtended>& sch,
                           std::time_t ad_on,
                           std::time_t ex_on)
    : id(i), owner(o), text(t), active(a), type(ty), tpoints_str(tp_str), wdays_str(wd_str), schedule(sch), added_on(ad_on), expiring_on(ex_on) {}
