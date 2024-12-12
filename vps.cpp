#include "vps.h"

VPS::VPS(const std::string &u, std::int64_t i, std::int64_t o, const std::string& n, const std::string& l_o) : uuid(u), id(i), owner(o), name(n), last_output(l_o)
{
    try
    {
        fetch_info();
    }
    catch(...)
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
        cmd.execute(std::string("virsh domstats ") + uuid);
        break;
    case ACTION::SCREENSHOT:
        cmd.execute(std::string("virsh screenshot ") + uuid + " screenshots/" + std::to_string(id) + ".png");
        break;
    case ACTION::RENAME:
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
        cmd.execute(std::string("virsh save ") + uuid + ' ' + std::to_string(id) + ".hib --running");
        break;
    case ACTION::RESTORE:
        cmd.execute(std::string("virsh restore " + std::to_string(id) + ".hib --running"));
        break;
    case ACTION::STOP:
        cmd.execute(std::string("virsh destroy ") + uuid);
        break;
    case ACTION::START:
        cmd.execute(std::string("virsh start ") + uuid);
        break;
    case ACTION::SHOW:
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

void VPS::perform(ACTION a) noexcept
{
    try
    {

        if(a != ACTION::INFO)
        {
            auto cmd = virsh_exec(a);
            if(!cmd.ExitStatus)
            {
                last_output =
R"(✅
```Output
)" + cmd.StdOut + R"(
```)";
            }
            else
            {
                last_output =
R"(⚠️
```Error
)" + cmd.StdErr + R"(
```)";
            }
        }
        else
        {
            last_output = R"(✅ _Information has been updated\!_)";
        }

        fetch_info();
    }

    catch(...)
    {
        last_output = R"(```
Internal error occured: the VPS doesn't exist\. Contact the hoster\.
```)";
    }
}

std::string VPS::string_state(STATE s) noexcept
{
    switch(s)
    {
    case STATE::RUNNING:
        return "Running";

    case STATE::BLOCKED:
        return "Blocked";

    case STATE::PAUSED:
        return "Paused";

    case STATE::SHUTDOWN:
        return "Shutdown";

    case STATE::CRASHED:
        return "Crashed";

    case STATE::DYING:
        return "Dying";
    }

    return "Undefined";
}

void VPS::fetch_info()
{
    auto cmd = virsh_exec(ACTION::INFO);
    boost::regex reg;

    if(!cmd.ExitStatus)
    {
        reg = R"((?<=Domain: ').*(?='))";
        name = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)->str();

        reg = R"((?<=state=)\d+)";
        state = static_cast<STATE>(std::stoi(boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)->str()));

        reg = R"((?<=vcpu.maximum=)\d+)";
        cpu_count = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)->str();

        reg = R"((?<=balloon.maximum=)\d+)";
        ram = std::to_string(std::stol(boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)->str()) >> 10) + " MiB";

        try
        {
            reg = R"(block.\d+.name=\K\w+)";
            auto names_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

            reg = R"(block.\d+.capacity=\K\w+)";
            auto first_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

            reg = R"(block.\d+.allocation=\K\w+)";
            auto second_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

            boost::sregex_iterator end_it;

            for(; first_it != end_it; ++names_it, ++first_it, ++second_it)
            {
                std::string capacity = std::to_string(std::stol(first_it->str()) >> 30);
                std::string allocation = std::to_string(std::stol(second_it->str()) >> 30);

                if((std::find_if(blocks.begin(),
                                 blocks.end(),
                [&names_it](const std::pair<std::string, std::string>& pair)
                {
                    return names_it->str() == pair.first;
                })
                        == blocks.end()))
                    blocks.push_back(
                            {names_it->str(), allocation  + " GiB / " + capacity + " GiB"}
                            );
            }

            if(state != VPS::STATE::CRASHED && state != VPS::STATE::DYING && state != VPS::STATE::BLOCKED)
            {
                reg = R"(net.\d+.name=\K\w+)";
                names_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

                reg = R"(net.\d+.rx.bytes=\K\w+)";
                first_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

                reg = R"(net.\d+.tx.bytes=\K\w+)";
                second_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

                for(; first_it != end_it; ++names_it, ++first_it, ++second_it)
                {
                    std::string download = std::to_string(std::stol(first_it->str()) >> 20);
                    std::string upload = std::to_string(std::stol(second_it->str()) >> 20);

                    if((std::find_if(netifstat.begin(),
                                     netifstat.end(),
                    [&names_it](const std::pair<std::string, std::string>& pair)
                    {
                        return names_it->str() == pair.first;
                    })
                            == netifstat.end()))
                        netifstat.push_back(
                                {names_it->str(), "Upload: " + upload  + " MiB / Download: " + download + " MiB"}
                                );
                }
            }
        }
        catch(...)
        {

        }
    }
    else
        throw std::exception();
}
