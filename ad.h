#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <ctime>

class Ad
{
public:
    typedef std::shared_ptr<Ad> Ptr;

    std::int64_t id;
    std::string owner;
    std::string text;
    bool active;
    std::int64_t added_on;
    std::int64_t expiring_on;
    std::vector<std::tm> schedule;

    Ad(const std::int64_t& = 0,
       const std::string& = std::string(),
       const std::string& = std::string(),
       bool = false,
       const std::int64_t& = 0,
       const std::int64_t& = 0);
};
