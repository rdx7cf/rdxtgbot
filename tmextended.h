#pragma once

#include <ctime>

struct TmExtended : std::tm
{
    bool executed; // Если изначально значение будет true, то бот не выкинет разом все рекламы на старте.

    TmExtended(bool exec = true) : executed(exec) {}
    TmExtended(std::tm t, bool exec = true) : std::tm(t), executed(exec) {}
};
