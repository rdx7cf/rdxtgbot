#include "VPS.h"
#include "Auxiliary.h"
#include "BashCommand.h"

// VPS

VPS::VPS(const std::string& u,
         std::int64_t i,
         std::int64_t o,
         const std::string& address,
         const std::string& login,
         const std::string& password,
         const std::string& n,
         const std::string& l_o): uuid_(u), id_(i), owner_(o), address_(address), login_(login), password_(password), name_(n), last_output_(l_o)
{
    try
    {
        blocks_.reserve(16);
        netifstat_.reserve(16);
        fetch_info();
    }
    catch(...)
    {
        last_output_ = R"(```
Internal error occured: the VPS doesn't exist\. Contact the hoster\.
```)";
    }
}

bool VPS::operator==(const VPS& rhs) const
{
    if(uuid_ != rhs.uuid_)
        return false;

    return true;
}

/*bool VPS::updateNeeded(const VPS& rhs) const
{
    if(id_ != rhs.id_)
        return true;
    if(owner_ != rhs.owner_)
        return true;
    if(name_ != rhs.name_)
        return true;
    if(state_ != rhs.state_)
        return true;
    if(cpu_count_ != rhs.cpu_count_)
        return true;
    if(ram_ != rhs.ram_)
        return true;
    if(blocks_ != rhs.blocks_)
        return true;
    if(netifstat_ != rhs.netifstat_)
        return true;
    if(screenshot_ != rhs.screenshot_)
        return true;
    if(last_output_ != rhs.last_output_)
        return true;

    return false;
}

VPS& VPS::operator=(const VPS& rhs)
{
    if(this == &rhs)
        return *this;

    uuid_ = rhs.uuid_;
    id_ = rhs.id_;
    owner_ = rhs.owner_;
    name_ = rhs.name_;
    state_ = rhs.state_;
    cpu_count_ = rhs.cpu_count_;
    ram_ = rhs.ram_;
    blocks_ = rhs.blocks_;
    netifstat_ = rhs.netifstat_;
    screenshot_ = rhs.screenshot_;
    last_output_ = rhs.last_output_;

    return *this;
}*/


BashCommand VPS::virsh_exec(ACTION a, const std::string& input) noexcept
{
    BashCommand cmd;

    switch(a)
    {
    case ACTION::INFO:
        cmd.execute(std::string("virsh domstats ") + uuid_);
        break;
    case ACTION::SCREENSHOT:
    {
        std::string filename = "vps/screenshots/" + std::to_string(id_) + ".png";
        cmd.execute(std::string("virsh screenshot ") + uuid_ + " " + filename);
        if(!cmd.exit_status_)
            screenshot_ = filename;
        else
            screenshot_.clear();
        break;
    }
    case ACTION::RENAME:
        name_ = input;
        cmd.execute(std::string("virsh domrename ") + uuid_ + ' ' + '"' + name_ + '"');
        break;
    case ACTION::REBOOT:
        cmd.execute(std::string("virsh destroy ") + uuid_ + " && virsh start " + uuid_);
        break;
    case ACTION::SUSPEND:
        cmd.execute(std::string("virsh suspend ") + uuid_);
        break;
    case ACTION::RESUME:
        cmd.execute(std::string("virsh resume ") + uuid_);
        break;
    case ACTION::RESET:
        cmd.execute(std::string("virsh reset ") + uuid_);
        break;
    case ACTION::SAVE:
        cmd.execute(std::string("virsh save ") + uuid_ + " vps/hibernate/" + std::to_string(id_) + ".hib --running");
        break;
    case ACTION::RESTORE:
        cmd.execute(std::string("virsh restore vps/hibernate/" + std::to_string(id_) + ".hib --running"));
        break;
    case ACTION::STOP:
        cmd.execute(std::string("virsh destroy ") + uuid_);
        break;
    case ACTION::START:
        cmd.execute(std::string("virsh start ") + uuid_);
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

void VPS::perform(ACTION a, const std::string& input) noexcept
{
    try
    {

        if(a != ACTION::INFO)
        {
            auto cmd = virsh_exec(a, input);
            if(!cmd.exit_status_)
            {
                last_output_ =
R"(✅
```Output
)" + cmd.std_out_ + R"(
```)";
            }
            else
            {
                last_output_ =
R"(⚠️
```Error
)" + cmd.std_err_ + R"(
```)";
            }
        }
        else
        {
            last_output_ = R"(✅ _Information has been updated\!_)";
        }

        fetch_info();
    }

    catch(...)
    {
        last_output_ = R"(```
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

    if(!cmd.exit_status_)
    {
        reg = R"((?<=Domain: ').*(?='))";
        name_ = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg)->str();

        reg = R"((?<=state=)\d+)";
        state_ = static_cast<STATE>(std::stoi(boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg)->str()));

        reg = R"((?<=vcpu.maximum=)\d+)";
        cpu_count_ = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg)->str();

        reg = R"((?<=balloon.maximum=)\d+)";
        ram_ = std::to_string(std::stol(boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg)->str()) >> 10) + " MiB";

        reg = R"(block.\d+.capacity=\K\w+)";
        auto first_it = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg);

        boost::sregex_iterator end_it;

        if(first_it != end_it)
        {
            reg = R"(block.\d+.allocation=\K\w+)";
            auto second_it = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg);


            reg = R"(block.\d+.name=\K\w+)";
            auto names_it = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg);

            blocks_.clear();

            for(; first_it != end_it; ++names_it, ++first_it, ++second_it)
            {
                std::string capacity = std::to_string(std::stol(first_it->str()) >> 30);
                std::string allocation = std::to_string(std::stol(second_it->str()) >> 30);


                blocks_.push_back(
                            std::string("▸ *") + names_it->str() + "* — __A__: " + allocation  + " GiB / __C__: " + capacity + " GiB"
                            );
            }

            if(state_ != VPS::STATE::CRASHED && state_ != VPS::STATE::DYING && state_ != VPS::STATE::BLOCKED)
            {
                reg = R"(net.\d+.name=\K\w+)";
                names_it = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg);

                reg = R"(net.\d+.rx.bytes=\K\w+)";
                first_it = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg);

                reg = R"(net.\d+.tx.bytes=\K\w+)";
                second_it = boost::sregex_iterator(cmd.std_out_.begin(), cmd.std_out_.end(), reg);

                netifstat_.clear();

                for(; first_it != end_it; ++names_it, ++first_it, ++second_it)
                {
                    std::string download = std::to_string(std::stol(first_it->str()) >> 20);
                    std::string upload = std::to_string(std::stol(second_it->str()) >> 20);


                    netifstat_.push_back(
                                std::string("▸ *") + names_it->str() + "* — __U__: " + upload  + " MiB / __D__: " + download + " MiB"
                                );
                }
            }
        }
    }
    else
        throw std::exception();
}
