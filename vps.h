#pragma once

#include <memory>
#include <string>
#include <boost/regex.hpp>

#include "bashcommand.h"

class VPS
{
public:

    using Ptr = std::shared_ptr<VPS>;
    using blockvec = std::vector<std::pair<std::string, std::string>>;

    enum class ACTION {
        INFO = 0, RENAME,                                                   // 0-9: Information management.
        STOP = 10, START, REBOOT, SAVE, RESTORE, RESET, RESUME, SUSPEND,    // 10-19: Power management.
        SHOW = 20                                                           // 20-: Backup management.
                      };

    enum class STATE  {RUNNING = 1, BLOCKED, PAUSED, SHUTDOWN, CRASHED, DYING};

    std::string uuid;
    std::int64_t id;
    std::int64_t owner;
    std::string name;
    STATE state;
    std::string cpu_count;
    std::string ram;
    blockvec blocks;

    mutable std::string last_output;

    VPS(
            const std::string& u,
            std::int64_t i = 0,
            std::int64_t o = 0,
            const std::string& n = std::string(),
            const std::string& l_o = R"(👁‍ _Awaiting new actions\.\.\._)"
            );


    void perform(ACTION) noexcept;
    static std::string string_state(STATE) noexcept;

private:
    BashCommand virsh_exec(ACTION) noexcept;
    BashCommand virsh_exec(const std::string&) noexcept;

    void fetch_info();
};
