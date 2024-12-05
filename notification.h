#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "tmextended.h"

class Notification
{
public:
    typedef std::shared_ptr<Notification> Ptr;

    std::int64_t id;
    std::string owner;
    std::string text;
    bool active;
    bool is_ad;
    std::time_t added_on;
    std::time_t expiring_on;
    std::vector<TmExtended> schedule;
    std::string tpoints_str;
    std::string wdays_str;

    Notification(std::int64_t = 0,
       const std::string& = std::string(),
       const std::string& = std::string(),
       bool = false,
       bool = false,
       std::time_t = 0,
       std::time_t = 0);
};
