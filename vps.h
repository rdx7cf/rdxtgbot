#pragma once

#include <memory>
#include <string>
#include <boost/regex.hpp>

#include "bashcommand.h"

class VPS
{
public:

    using Ptr = std::shared_ptr<VPS>;

    enum class ACTION {INFO = 0, STOP, START, REBOOT, SAVE, RESTORE, RESET, RESUME, SUSPEND, NAME};

    std::string uuid;
    std::int64_t id;
    std::int64_t owner;
    std::string name;
    std::string state;
    std::string cpu_count;
    std::string ram;

    mutable std::string last_output;

    VPS(
            const std::string& u,
            std::int64_t i = 0,
            std::int64_t o = 0,
            const std::string& n = std::string(),
            const std::string& l_o = std::string()
            );


    std::string perform(ACTION) noexcept;

private:
    BashCommand virsh_exec(ACTION) noexcept;
    BashCommand virsh_exec(const std::string&) noexcept;
    void fetch_info();
};
