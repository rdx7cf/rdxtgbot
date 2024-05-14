#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <thread>
#include <sstream>


#include <tgbot/tgbot.h>

#include "bot_commands.h"
#include "database.h"
#include "filesystem.h"
#include "multithreading.h"

// Probably it would be a good idea to inherit from Tg::Bot a custom User class in order to save last user's message.
// The temporary solution for saving the users database is done. However it's not that effective as I want. If someone decide to spam the bot with messages, it will permanently be comparing names...
// Most likely I'm gonna use some sort of SQL DB. Or just a text file?

// So I came up with a SQLite solution.
// I need to figure out multithreading.
// What functions should be async? (Control Panel, DB saving, maybe DB syncing)
// Before saving Data, make a backup (boost::filesystem)!
// Maybe it's worth to merge all the commands into onAnyMessage() then use ifs.

// A specific atomic<bool> to stop all threads?
// https://codereview.stackexchange.com/questions/283622/safely-starting-and-stopping-a-thread-owned-by-a-class
// Maybe a custom class which contains a static atomic<bool> flag...
// https://stackoverflow.com/questions/23117354/how-to-stop-an-stdthread-from-running-without-terminating-the-program
// Or just use std::jthread...
// Thread pool is required.


// I have to figure out the multithreading file access. // Done

// What's remaining:
// 0. Write "fast query" function;
// 1. Finish the bot commands;
// 2. Write a database syncing function;
// 3. Finish the control panel.



int main(int argc, char** argv)
{
    if(agrc < 2)
        return 0;
    std::srand(std::time(nullptr));

    std::vector<TgBot::User::Ptr> users;

    if(db_readOnStart("./sqlite_db/x7cf.db", users)) return 1;

    TgBot::Bot bot(argv[1]);


    std::array<std::jthread, 2> thread_pool
    {
        std::jthread(thread_long_polling, std::ref(bot), std::ref(users)),
        std::jthread(thread_db_syncing)
    };

    std::cout << "Bot username: " << bot.getApi().getMe()->username << std::endl;
    std::cout << "Send 4 to quit: ";
    int choice;
    while(true)
    {
        if(!(std::cin >> choice)) // std::cout / std::cin should be used by a SINGLE thread ONLY.
        {

            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');


            std::cout << "Correct answers only: ";

        }
        switch(choice)
        {
        case 4:
            return 0;
        }
    }
    // The program will call request_stop() and then join() for each of the threads.
}
