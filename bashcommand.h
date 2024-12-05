#pragma once
#include <string>
#include <array>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <cerrno>
#include <clocale>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>

class BashCommand // https://dev.to/aggsol/calling-shell-commands-from-c-8ej
{
public:
    int             ExitStatus = 0;
    std::string     Command;
    std::string     StdIn;
    std::string     StdOut;
    std::string     StdErr;

    void execute();
};
