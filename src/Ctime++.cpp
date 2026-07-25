#include "Ctime++.h"

bool TmExtended::operator==(const TmExtended& rhs) const
{
    if(executed_ != rhs.executed_)
        return false;

    if(tm_sec != rhs.tm_sec)
        return false;

    if(tm_min != rhs.tm_min)
        return false;

    if(tm_hour != rhs.tm_hour)
        return false;

    if(tm_mday != rhs.tm_mday)
        return false;

    if(tm_mon != rhs.tm_mon)
        return false;

    if(tm_year != rhs.tm_year)
        return false;

    if(tm_wday != rhs.tm_wday)
        return false;

    if(tm_yday != rhs.tm_yday)
        return false;

    if(tm_isdst != rhs.tm_isdst)
        return false;

    if(tm_gmtoff != rhs.tm_gmtoff)
        return false;

    if(tm_zone != rhs.tm_zone)
        return false;



    return true;
}

std::tm localtimeTs(std::time_t timer)
{
    std::tm tm {};
#ifdef __unix__
    localtime_r(&timer, &tm);
#elif _WIN32
    localtime_s(&timer, &tm);
#endif
    return tm;
}
