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
std::string generateMessage(const std::vector<std::pair<std::string, T>>& blocks,
                                   std::string indent = std::string())
{
    std::string message = "";    

    std::for_each(blocks.begin(), blocks.end(), [&message, &indent](const std::pair<std::string, T>& block)
    {
        message += indent + block.first + '\n';

        if constexpr(std::is_same_v<T, std::vector<std::string>>)
        {
            for(auto const& row : block.second)
                message += indent + "\t\t" + row + '\n';
        }
        else
        {
            if(!block.second.empty())
                message += generateMessage(block.second, indent + "\t\t");
        }
    });

    return message;
}

        /*std::vector<
                std::pair<std::pair<std::string, std::string>, std::vector<
                std::pair<std::string, std::string>>>> vec =

        {     // vector< pair<string,string> , ... >
            { // pair<string, string> // vector<pair<string, string>, vector...>
                {
                    {"*__VPS Information__*", {}}, {}
                },
                {
                    {"■ *Name*:", std::string("`") + vps->name_ + "`"}, {}
                },
                {
                    {"■ *UUID*:", std::string("`") + vps->uuid_ + "`"}, {}
                },
                {
                    {"■ *State*:", std::string("__") + VPS::string_state(vps->state_) + "__"}, {}
                },
                {
                    {"■ *Threads*:", vps->cpu_count_}, {}
                },
                {
                    {"■ *RAM*:", vps->ram_}, {}
                },
                {
                    {"■ *Storages*:", {}},
                    vps->blocks_
                },
                {
                    {"■ *Network*:", {}},
                    vps->netifstat_
                }
            }
        };
        */



std::string shortenString(const std::string& str, std::string::size_type desired_sz, bool ellipsis = true) noexcept;
}



#endif
