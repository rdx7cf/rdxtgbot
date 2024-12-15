#ifndef LOGGER_H
#define LOGGER_H


#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>

#include "Ctime++.h"


struct Logger
{
    Logger() = delete;

    static std::string filename_;
    static void write(const std::string& message);
};

#endif
