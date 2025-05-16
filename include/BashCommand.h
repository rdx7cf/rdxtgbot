#ifndef BASHCOMMAND_H
#define BASHCOMMAND_H

#include <string>

class BashCommand // https://dev.to/aggsol/calling-shell-commands-from-c-8ej
{
public:
    int             exit_status_ = 0;
    std::string     std_in_;
    std::string     std_out_;
    std::string     std_err_;

    void execute(const std::string& Command);
};

#endif
