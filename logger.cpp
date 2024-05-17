#include "logger.h"

std::string Logger::filename_ {"./log.log"};

void Logger::write(const std::string& message)
{
    std::ofstream file(filename_, std::ios_base::out | std::ios_base::app);


    std::time_t now = std::time(nullptr);

    file <<
     "[" << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << "] "
         << message << std::endl;

    file.close();
}
