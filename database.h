#pragma once

#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <stdexcept>


#include <tgbot/tgbot.h>
#include <boost/filesystem.hpp>
#include <sqlite3.h>

#include "to_filelog.h"
#include "multithreading.h"

class sqlite3_exception : public std::runtime_error
{
public:
    sqlite3_exception(const std::string& what_arg) : std::runtime_error(what_arg) {}
};

int db_fast_query(const char*, const char*);
int db_readOnStart(const char*, std::vector<TgBot::User::Ptr>&);
int db_save(const char*, std::vector<TgBot::User::Ptr>&, std::ostream&);
void db_sync(const char*, std::vector<TgBot::User::Ptr>&, std::ostream&, unsigned);




