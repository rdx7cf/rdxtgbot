#pragma once

#include <cstdio>
#include <ctime>
#include <iomanip>

#include <boost/filesystem.hpp>

#include "to_filelog.h"
#include "multithreading.h"

void make_backup(const std::string&);

;

// Write a function that will be deleting the oldest backups of the database
// (the backups will be stored in a separated directory;
// two integers will be passed to the function: the number of backups to delete & the time between iterations)

