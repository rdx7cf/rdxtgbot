#include "Logger.h"
#include "Ctime++.h"
#include <fstream>
#include <ctime>
#include <iomanip>


std::string Logger::filename_ {"./log.log"};

void Logger::write(const std::string& message)
{
    std::ofstream file(filename_, std::ios_base::out | std::ios_base::app);

    if(!file.is_open())
        throw std::runtime_error("Unable to open the log file '" + filename_ + "'.\n");

    std::tm now = localtimeTs(std::time(nullptr));

    file <<
     "[" << std::put_time(&now, "%d-%m-%Y %H:%M:%S") << "] "
         << message << std::endl;

    file.close();
}
