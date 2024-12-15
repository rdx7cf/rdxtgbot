#include "Auxiliary.h"

namespace AUX
{
std::string shortenString(const std::string& str, std::string::size_type desired_sz, bool ellipsis) noexcept
{
    auto sz = str.size();

    if(sz >= desired_sz)
        return std::string(str, 0, ellipsis ? desired_sz - 3 : desired_sz) + (ellipsis ? std::string("...") : "");
    else if(str[sz - 1] == '\n')
        return std::string(str, 0, sz - 1);
    else
        return str;
}

}
