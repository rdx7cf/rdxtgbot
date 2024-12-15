#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "tmextended.h"

class Notification
{
public:
    using Ptr = std::shared_ptr<Notification>;
    enum class TYPE {SYSTEM = -1, COMMERCIAL, CURRENCY};

    std::int64_t id;
    std::string owner;
    std::string text;
    bool active;
    TYPE type;
    std::string tpoints_str;
    std::string wdays_str;
    std::vector<TmExtended> schedule;
    std::time_t added_on;
    std::time_t expiring_on;

    Notification(
        std::int64_t = 0,
        const std::string& = std::string(),
        const std::string& = std::string(),
        bool = false,
        TYPE = static_cast<TYPE>(-1),
        const std::string& = std::string(),
        const std::string& = std::string(),
        const std::vector<TmExtended>& = std::vector<TmExtended>(),
        std::time_t = 0,
        std::time_t = 0);
};
