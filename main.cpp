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

#include "database.h"

#include "bot_commands.h"
#include "database_old.h"
#include "filesystem.h"
#include "multithreading.h"

// Probably it would be a good idea to inherit from Tg::Bot a custom User class in order to save last user's message.
// The temporary solution for saving the users database is done. However it's not that effective as I want. If someone decide to spam the bot with messages, it will permanently be comparing names...
// Most likely I'm gonna use some sort of SQL DB. Or just a text file?

// So I came up with the SQLite solution.
// I have to figure out multithreading.
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

// Is sync needed if the bot will insert each user separetely?

// Зашёл новый пользователь => INSERT INTO. Создание новой копии базы данных (?)
// Синхронизацию стоит заменить на копирование. Как только в отдельной папки под бэкапы > 5 копий, удалить самую старую.

void thread_long_polling(std::stop_token, TgBot::Bot&, const std::unique_ptr<Database>&);


int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cout << "USAGE:\ntgbot '[API_TOKEN]' '[PATH_TO_DATABASE]'\n";
        return 1;
    }

    std::unique_ptr<Database> database(new Database(argv[2], to_filelog));
    /*try
    {

    }
    catch (const std::exception& ex)
    {
        std::cout << "PROCESS TERMINATED: " << ex.what() << std::endl;

        return 1;
    }*/


    TgBot::Bot bot(argv[1]);


    std::jthread long_polling(thread_long_polling, std::ref(bot), std::cref(database));

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


void thread_long_polling(std::stop_token tok, TgBot::Bot& bot, const std::unique_ptr<Database>& database)
{
    /*bot.getEvents().onAnyMessage([&database, &bot](TgBot::Message::Ptr message)
                                 { anymsg(message, bot, database); });*/

    bot.getEvents().onCommand("start",
                              [&database, &bot](TgBot::Message::Ptr message)
                              { start(message, bot, database); });


    TgBot::TgLongPoll longPoll(bot);

    while (true)
    {
        if(tok.stop_requested())
        {
            //std::cout << "Stop requested." << std::endl;
            return;
        }
        try
        {
            longPoll.start();
        }
        catch (TgBot::TgException& e)
        {
            //std::cerr << "ERROR " << int(e.errorCode) << " : " << e.what() << std::endl;
        }
    }

}
