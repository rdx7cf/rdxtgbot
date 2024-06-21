#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "tmextended.h"

class Ad
{
public:
    typedef std::shared_ptr<Ad> Ptr;

    std::int64_t id;
    std::string owner;
    std::string text;
    bool active;
    std::time_t added_on;
    std::time_t expiring_on;
    std::vector<TmExtended> schedule;
    std::string tpoints_str;
    std::string wdays_str;

    Ad(const std::int64_t& = 0,
       const std::string& = std::string(),
       const std::string& = std::string(),
       bool = false,
       const std::time_t& = 0,
       const std::time_t& = 0);
};
