#ifndef VPS_H
#define VPS_H

#include <memory>
#include <string>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include <tgbot/tgbot.h>


#include "BashCommand.h"
#include "Auxiliary.h"

/*class Backup
{
    class File
    {
        std::string filename_;
        std::int64_t size_bytes_;

        std::int64_t
    };


};*/


class VPS
{
public:
    using Ptr = std::shared_ptr<VPS>;

    enum class ACTION {
        INFO = 0, SCREENSHOT, RENAME,                                                   // 0-9: Information management.
        STOP = 10, START, REBOOT, SAVE, RESTORE, RESET, RESUME, SUSPEND,    // 10-19: Power management.
        SHOW = 20                                                           // 20-: Backup management.
                      };

    enum class STATE  {RUNNING = 1, BLOCKED, PAUSED, SHUTDOWN, CRASHED, DYING};

    std::string uuid_;
    std::int64_t id_;
    std::int64_t owner_;
    std::string address_;
    std::string login_;
    std::string password_;
    std::string name_;
    STATE state_;
    std::string cpu_count_;
    std::string ram_;
    std::vector<std::string> blocks_;
    std::vector<std::string> netifstat_;



    std::string screenshot_;

    mutable std::string last_output_;

    VPS(const std::string& u = std::string(),
        std::int64_t i = 0,
        std::int64_t o = 0,
        const std::string& address = "",
        const std::string& login = "",
        const std::string& password = "",
        const std::string& n = "",
        const std::string& l_o = R"(👁‍ _Awaiting new actions\.\.\._)");


    void perform(ACTION, const std::string& = std::string()) noexcept;
    static std::string string_state(STATE) noexcept;

    bool operator==(const VPS&) const;
    /*bool updateNeeded(const VPS&) const;
    VPS& operator=(const VPS&);*/

private:
    BashCommand virsh_exec(ACTION, const std::string& = std::string()) noexcept;
    BashCommand virsh_exec(const std::string&) noexcept;


    void fetch_info();
};

#endif
