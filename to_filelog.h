#pragma once

#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <thread>

#include "multithreading.h"

void to_filelog(const std::string&, const std::string& path = "./logs/log.log");
