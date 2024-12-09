#pragma once

#include <memory>
#include <string>
#include <boost/regex.hpp>

#include "bashcommand.h"

class VPS
{
public:

    using Ptr = std::shared_ptr<VPS>;

    enum class ACTION {INFO, REBOOT, SUSPEND, RESUME, RESET, SAVE, RESTORE, STOP, START, NAME};

    std::string uuid;
    std::int64_t id;
    std::int64_t owner;
    std::string name;
    std::string state;

    VPS(
            const std::string& u,
            std::int64_t i = 0,
            std::int64_t o = 0,
            const std::string& n = std::string()
            );


    std::string perform(ACTION) const noexcept;

private:
    BashCommand virsh_exec(ACTION) const noexcept;
};
