#include "ctime++.h"

std::tm localtime_ts(std::time_t timer)
{
    std::tm t {};
#ifdef __unix__
    localtime_r(&timer, &t);
#elif _WIN32
    localtime_s(&timer, &t);
#endif
    return t;
}
