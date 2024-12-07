#include "vps.h"

std::string VPS::perform(ACTION a)
{
    std::string result;

    BashCommand cmd;

    switch(a)
    {
    case ACTION::INFO:
        cmd.execute(std::string("virsh dominfo ") + name);
        break;
    case ACTION::REBOOT:
        cmd.execute(std::string("virsh destroy ") + name + " && virsh start " + name);
        break;
    case ACTION::SUSPEND:
        cmd.execute(std::string("virsh suspend ") + name);
        break;
    case ACTION::RESUME:
        cmd.execute(std::string("virsh resume ") + name);
        break;
    case ACTION::RESET:
        cmd.execute(std::string("virsh reset ") + name);
        break;
    case ACTION::SAVE:
        cmd.execute(std::string("virsh save ") + name);
        break;
    case ACTION::RESTORE:
        cmd.execute(std::string("virsh restore ") + name);
        break;
    case ACTION::STOP:
        cmd.execute(std::string("virsh stop ") + name);
        break;
    case ACTION::START:
        cmd.execute(std::string("virsh start ") + name);
        break;
    }

    if(!cmd.ExitStatus)
    {
        result =
R"(
*Success!*
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
