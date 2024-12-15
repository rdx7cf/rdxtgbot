#ifndef AUXILIARY_H
#define AUXILIARY_H

#include <type_traits>
#include <string>
#include <vector>
#include <type_traits>
#include <algorithm>

namespace AUX
{
template<typename T>
constexpr std::underlying_type_t<T> toUnderlying(T o)
{
    return static_cast<std::underlying_type_t<T>>(o);
}

template<typename T>
static std::string generateMessage(const std::vector<std::pair<std::pair<std::string, std::string>, T>>& blocks,
                                   std::string indent = std::string())
{
    std::string message = "";    

    std::for_each(blocks.begin(), blocks.end(), [&message, &indent](const std::pair<std::pair<std::string, std::string>, T>& block)
        {
            message += indent + block.first.first + ": " + block.first.second + '\n';

            if constexpr(std::is_same_v<T, std::vector<std::pair<std::string, std::string>>>)
            {
                for(auto const& row : block.second)
                    message += indent + "\t\t" + row.first + ": " + row.second + '\n';
            }
            else
            {
                if(!block.second.empty())
                    message += generateMessage(block.second, indent + "\t\t");
            }
        });

        return message;
}

std::string shortenString(const std::string& str, std::string::size_type desired_sz, bool ellipsis = true) noexcept;
}



#endif
