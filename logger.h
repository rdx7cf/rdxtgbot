#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>

#include "ctime++.h"


class Logger
{
public:
    static std::string filename_;
    static void write(const std::string& message);
private:
    Logger();
};
