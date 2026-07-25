cmake_minimum_required(VERSION 3.16)

project(rdxtgbot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(
    HAVE_CURL
    TGBOT_DISABLE_NAGLES_ALGORITHM
    TGBOT_CHANGE_SOCKET_BUFFER_SIZE
    TGBOT_CHANGE_READ_BUFFER_SIZE
)

add_compile_options(-Wall)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(CURL REQUIRED)

# Собираем библиотеку вместе с проектом
add_subdirectory(tgbot-cpp)

add_executable(rdxtgbot
    src/main.cpp

    src/Auxiliary.cpp
    src/BashCommand.cpp
    src/BotExtended.cpp
    src/Ctime++.cpp
    src/Logger.cpp
    src/Notification.cpp
    src/SQLFile.cpp
    src/UserExtended.cpp
    src/VPS.cpp
    src/Table.cpp
    src/BotAction.cpp
    src/Backup.cpp
)

target_include_directories(rdxtgbot PRIVATE
    include
)

target_link_libraries(rdxtgbot PRIVATE
    TgBot
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
    Boost::system
    Boost::filesystem
    Threads::Threads
)