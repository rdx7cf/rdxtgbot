#include "to_filelog.h"

void to_filelog(const std::string& message, const std::string& path)
{
    std::ofstream file(path, std::ios_base::out | std::ios_base::app);


    std::time_t now = std::time(nullptr);

    file <<
     "[" << std::put_time(std::localtime(&now), "%d-%m-%Y %H-%M-%S") << "] "
         << message << std::endl;

    file.close();
}
