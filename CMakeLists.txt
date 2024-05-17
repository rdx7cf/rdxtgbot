cmake_minimum_required(VERSION 3.10.2)
project(tgbot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DTGBOT_DISABLE_NAGLES_ALGORITHM -DTGBOT_CHANGE_SOCKET_BUFFER_SIZE -DTGBOT_CHANGE_READ_BUFFER_SIZE")
set(Boost_USE_MULTITHREADED ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(CURL)
include_directories(/usr/local/include ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${SQLite3_INCLUDE_DIR})
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()
add_executable(tgbot main.cpp
    bot_commands.h  bot_commands.cpp

    database.h database.cpp
    myhttpclient.h
    logger.h logger.cpp

)

target_link_libraries(tgbot libTgBot.a ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${SQLite3_LIBRARIES} ${CURL_LIBRARIES})
