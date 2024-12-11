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

        /*if(state == "работает")
        {
            virsh_exec("screenshot " + name + " vps_sshots/" + name + ".jpeg");
        }*/

        reg = R"((?<=vcpu.maximum=)\d+)";
        cpu_count = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)->str();

        reg = R"((?<=balloon.maximum=)\d+)";
        ram = std::to_string(std::stol(boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg)->str()) / 1048576) + " GiB";

        reg = R"(block.\d+.name=\K\w+)";
        auto block_names_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

        reg = R"(block.\d+.capacity=\K\w+)";
        auto block_capacities_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

        reg = R"(block.\d+.allocation=\K\w+)";
        auto block_allocations_it = boost::sregex_iterator(cmd.StdOut.begin(), cmd.StdOut.end(), reg);

        boost::sregex_iterator end_it;

        while(block_capacities_it != end_it)
        {
            std::string capacity = std::to_string(std::stol(block_capacities_it->str()) / 1073741824);
            std::string allocation = std::to_string(std::stol(block_allocations_it->str()) / 1073741824);

            if((std::find_if(blocks.begin(),
                             blocks.end(),
            [&block_names_it](const std::pair<std::string, std::string>& pair)
            {
                return block_names_it->str() == pair.first;
            })
                    == blocks.end()))
                blocks.push_back(
                        {block_names_it->str(), allocation  + " GiB / " + capacity + " GiB"}
                        );

            ++block_names_it;
            ++block_capacities_it;
            ++block_allocations_it;
        }

        //reg = R"(CPU:\s+\K\d+)";
    }
    else
        throw std::exception();
}
