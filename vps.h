#pragma once

#include <memory>
#include <string>

#include "bashcommand.h"

class VPS
{
public:

    using Ptr = std::shared_ptr<VPS>;

    enum class ACTION {INFO, REBOOT, SUSPEND, RESUME, RESET, SAVE, RESTORE, STOP, START};

    std::int64_t owner; // UserExtended::Ptr?

    std::int64_t id;
    std::string uuid;
    std::string name;


    std::string last_action;



    VPS(
            std::int64_t o = 0,
            std::int64_t i = 0,
            const std::string& u = std::string(),
            const std::string& n = std::string())
        : owner(o), id(i), uuid(u), name(n) {}


    std::string perform(ACTION); // ?
};
