#pragma once

#include <type_traits>

template<typename T>
constexpr std::underlying_type_t<T> to_underl(T o)
{
    return static_cast<std::underlying_type_t<T>>(o);
}
