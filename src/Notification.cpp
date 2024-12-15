#include "Notification.h"

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
    : id_(i), owner_(o), text_(t), active_(a), type_(ty), tpoints_str_(tp_str), wdays_str_(wd_str), schedule_(sch), added_on_(ad_on), expiring_on_(ex_on) {}

bool Notification::operator==(const Notification& rhs) const
{
    if(id_ != rhs.id_)
        return false;

    return true;
}

/*bool Notification::updateNeeded(const Notification& rhs) const
{
    if(owner_ != rhs.owner_)
        return true;
    if(text_ != rhs.text_)
        return true;
    if(active_ != rhs.active_)
        return true;
    if(type_ != rhs.type_)
        return true;
    if(tpoints_str_ != rhs.tpoints_str_)
        return true;
    if(wdays_str_ != rhs.wdays_str_)
        return true;
    if(schedule_ != rhs.schedule_)
        return true;
    if(added_on_ != rhs.added_on_)
        return true;
    if(expiring_on_ != rhs.expiring_on_)
        return true;

    return false;
}

Notification& Notification::operator=(const Notification& rhs)
{
    if(this == &rhs)
        return *this;

    id_ = rhs.id_;
    owner_ = rhs.owner_;
    text_ = rhs.text_;
    active_ = rhs.active_;
    type_ = rhs.type_;
    tpoints_str_ = rhs.tpoints_str_;
    wdays_str_ = rhs.wdays_str_;
    schedule_ = rhs.schedule_;
    added_on_ = rhs.added_on_;
    expiring_on_ = rhs.expiring_on_;

    return *this;
}*/
