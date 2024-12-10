#include "vps.h"

VPS::VPS(const std::string &u, std::int64_t i, std::int64_t o, const std::string& n, const std::string& l_o) : uuid(u), id(i), owner(o), name(n), last_output(l_o)
{
    try
    {
        fetch_info();
    }
    catch(const std::exception& exc)
    {
        last_output = R"(```
Internal error occured: the VPS doesn't exist\. Contact the hoster\.
```)";
    }
}

BashCommand VPS::virsh_exec(ACTION a) noexcept
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
        cmd.execute(std::string("virsh save ") + uuid + " hibernate --running");
        break;
    case ACTION::RESTORE:
        cmd.execute(std::string("virsh restore hibernate --running"));
        break;
    case ACTION::STOP:
        cmd.execute(std::string("virsh destroy ") + uuid);
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

BashCommand VPS::virsh_exec(const std::string& command) noexcept
{
    BashCommand cmd;
    cmd.execute(std::string("virsh ") + command);
    return cmd;
}

std::string VPS::perform(ACTION a) noexcept
{
    try
    {
        if(a != ACTION::INFO)
        {
            auto cmd = virsh_exec(a);
            if(!cmd.ExitStatus)
            {
                last_output =
R"(>*Success*\!
```Output
)" + cmd.StdOut + R"(
```)";
            }
            else
            {
                last_output =
R"(>*Something went wrong while attempting to perform the requested action on the VPS*\.
```Error
)" + cmd.StdErr + R"(
```)";
            }
        }

        fetch_info();
    }
    catch(const std::exception& exc)
    {
        last_output = R"(```
Internal error occured: the VPS doesn't exist\. Contact the hoster\.
```)";
    }



    return last_output;
}

void VPS::fetch_info()
{
    auto cmd = virsh_exec(ACTION::INFO);
    boost::regex reg;

    if(!cmd.ExitStatus)
    {
        reg = R"(Имя:\s+\K\w+(?=\n))";
        name = (*boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)).str();

        reg = R"(Состояние:\s*\K\D+(?=\n))";
        state = (*boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)).str();

        if(state == "работает")
        {
            virsh_exec("screenshot " + name + " vps_sshots/" + name + ".jpeg");
        }

        reg = R"(CPU:\s+\K\d+)";
        cpu_count = (*boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)).str();

        reg = R"(Макс\.память:\s+\K\d+)";
        ram = std::to_string(std::stol((*boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)).str()) / 1048576) + " GiB"; //std::to_string(std::stod((*boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)).str()) / 1048576) + " GiB";
        //(*boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)).str() + " KiB"
    }
    else
        throw std::exception();
}
