#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>

#include "Ctime++.h"

class Notification
{
public:
    using Ptr = std::shared_ptr<Notification>;
    enum class TYPE {SYSTEM = -1, COMMERCIAL, CURRENCY};

    std::int64_t id_;
    std::string owner_;
    std::string text_;
    bool active_;
    TYPE type_;
    std::string tpoints_str_;
    std::string wdays_str_;
    std::vector<TmExtended> schedule_;
    std::time_t added_on_;
    std::time_t expiring_on_;
    std::string parse_mode_;

    Notification(
        std::int64_t = 0,
        const std::string& = "",
        const std::string& = "",
        bool = false,
        TYPE = static_cast<TYPE>(-1),
        const std::string& = "",
        const std::string& = "",
        const std::vector<TmExtended>& = std::vector<TmExtended>(),
        std::time_t = 0,
        std::time_t = 0,
        const std::string& parse_mode = "");

    bool operator==(const Notification&) const;
    /*bool updateNeeded(const Notification&) const;
    Notification& operator=(const Notification&);*/
};

#endif
