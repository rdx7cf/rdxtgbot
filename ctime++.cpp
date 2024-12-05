#include "ctime++.h"

std::tm localtime_ts(std::time_t timer)
{
    std::tm tm {};
#ifdef __unix__
    localtime_r(&timer, &tm);
#elif _WIN32
    localtime_s(&timer, &tm);
#endif
    return tm;
}
