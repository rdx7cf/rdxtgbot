#pragma once
#include <string>
#include <cstdint>
#include <memory>

class Ad
{
public:
    typedef std::shared_ptr<Ad> Ptr;

    std::int64_t id;
    std::string owner;
    std::string text;
    bool active;
    std::int64_t expiring_on;

    Ad(const std::int64_t& = 0, const std::string& = std::string(), const std::string& = std::string(), bool = false, const std::int64_t& = 0);
};
