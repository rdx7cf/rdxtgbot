#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>

#include "ctime++.h"


struct Logger
{
    Logger() = delete;

    static std::string filename_;
    static void write(const std::string& message);
};
