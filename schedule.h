#pragma once

#include <vector>
#include <string>
#include <ctime>

class Schedule
{
public:
    Schedule();

    void add_tpoint(const std::string&);
    void add_wday(const std::string&);
    bool is_executed_today() { return executed; }

private:
    bool executed;
    std::vector<std::tm> tpoints;
    std::vector<int> wdays;
};
