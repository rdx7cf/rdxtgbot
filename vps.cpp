#include "vps.h"

VPS::VPS(const std::string &u, std::int64_t i, std::int64_t o, const std::string& n) : uuid(u), id(i), owner(o), name(n) {}

BashCommand VPS::virsh_exec(ACTION a) const noexcept
{
    BashCommand cmd;

    switch(a)
    {
    case ACTION::INFO:
        cmd.execute(std::string("virsh dominfo ") + uuid);
        break;
    case ACTION::REBOOT:
        cmd.execute(std::string("virsh destroy ") + uuid + " && virsh start " + uuid);
        break;
    case ACTION::SUSPEND:
        cmd.execute(std::string("virsh suspend ") + uuid);
        break;
    case ACTION::RESUME:
        cmd.execute(std::string("virsh resume ") + uuid);
        break;
    case ACTION::RESET:
        cmd.execute(std::string("virsh reset ") + uuid);
        break;
    case ACTION::SAVE:
        cmd.execute(std::string("virsh save ") + uuid);
        break;
    case ACTION::RESTORE:
        cmd.execute(std::string("virsh restore ") + uuid);
        break;
    case ACTION::STOP:
        cmd.execute(std::string("virsh stop ") + uuid);
        break;
    case ACTION::START:
        cmd.execute(std::string("virsh start ") + uuid);
        break;
    case ACTION::NAME:
        cmd.execute(std::string("virsh domname ") + uuid);
        break;
    }

    return cmd;
}

std::string VPS::perform(ACTION a) const noexcept
{
    std::string result;

    auto cmd = virsh_exec(a);

    if(!cmd.ExitStatus)
    {
        result =
R"(
*Success*\!
```stdout
)" + cmd.StdOut + R"(
```
)";
    }
    else
    {
        result =
R"(
*Something went wrong while attempting to perform the requested action on the VPS*\.
```stderr
)" + cmd.StdErr + R"(
```)";
    }

    return result;
}
