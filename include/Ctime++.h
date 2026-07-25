#ifndef CTIMEPLUSPLUS_H
#define CTIMEPLUSPLUS_H

#include <ctime>

struct TmExtended : std::tm
{
    bool executed_; // Если изначально значение будет true, то бот не выкинет разом все рекламы на старте.

    TmExtended(bool exec = true) : executed_(exec) {}
    TmExtended(std::tm t, bool exec = true) : std::tm(t), executed_(exec) {}

    bool operator==(const TmExtended& rhs) const;
};

std::tm localtimeTs(std::time_t);



#endif
