#include "BashCommand.h"
#include <array>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <cerrno>
#include <clocale>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include "Logger.h"

void BashCommand::execute(const std::string& Command)
{
    const int READ_END = 0;
    const int WRITE_END = 1;

    int infd[2] = {0, 0};
    int outfd[2] = {0, 0};
    int errfd[2] = {0, 0};

    auto cleanup = [&]() {
        ::close(infd[READ_END]);
        ::close(infd[WRITE_END]);

        ::close(outfd[READ_END]);
        ::close(outfd[WRITE_END]);

        ::close(errfd[READ_END]);
        ::close(errfd[WRITE_END]);
    };

    auto rc = ::pipe(infd);
    if(rc < 0)
    {
        throw std::runtime_error(std::strerror(errno));
    }

    rc = ::pipe(outfd);
    if(rc < 0)
    {
        ::close(infd[READ_END]);
        ::close(infd[WRITE_END]);
        throw std::runtime_error(std::strerror(errno));
    }

    rc = ::pipe(errfd);
    if(rc < 0)
    {
        ::close(infd[READ_END]);
        ::close(infd[WRITE_END]);

        ::close(outfd[READ_END]);
        ::close(outfd[WRITE_END]);
        throw std::runtime_error(std::strerror(errno));
    }

    auto pid = fork();
    if(pid > 0) // PARENT
    {
        ::close(infd[READ_END]);    // Parent does not read from stdin
        ::close(outfd[WRITE_END]);  // Parent does not write to stdout
        ::close(errfd[WRITE_END]);  // Parent does not write to stderr

        if(::write(infd[WRITE_END], std_in_.data(), std_in_.size()) < 0)
        {
            throw std::runtime_error(std::strerror(errno));
        }
        ::close(infd[WRITE_END]); // Done writing
    }
    else if(pid == 0) // CHILD
    {
        ::dup2(infd[READ_END], STDIN_FILENO);
        ::dup2(outfd[WRITE_END], STDOUT_FILENO);
        ::dup2(errfd[WRITE_END], STDERR_FILENO);

        ::close(infd[WRITE_END]);   // Child does not write to stdin
        ::close(outfd[READ_END]);   // Child does not read from stdout
        ::close(errfd[READ_END]);   // Child does not read from stderr

        ::execl("/bin/bash", "bash", "-c", Command.c_str(), nullptr);
        ::exit(EXIT_SUCCESS);
    }

    // PARENT
    if(pid < 0)
    {
        cleanup();
        throw std::runtime_error("Failed to fork");
    }

    int status = 0;
    ::waitpid(pid, &status, 0);

    std::array<char, 256> buffer;

    ssize_t bytes = 0;
    do
    {
        bytes = ::read(outfd[READ_END], buffer.data(), buffer.size());
        try
        {
            std_out_.append(buffer.data(), bytes);
        }
        catch(const std::exception& e)
        {
            std_out_ = "The output is probably too big...";
            Logger::write(std::string(": ERROR : CMD : ") + e.what() + ".");
            break;
        }
    }
    while(bytes > 0);

    do
    {
        bytes = ::read(errfd[READ_END], buffer.data(), buffer.size());
        try
        {
            std_err_.append(buffer.data(), bytes);
        }
        catch(const std::exception& e)
        {
            std_err_ = "The output is probably too big...";
            Logger::write(std::string(": ERROR : CMD : ") + e.what() + ".");
            break;
        }
    }
    while(bytes > 0);

    if(WIFEXITED(status))
    {
        exit_status_ = WEXITSTATUS(status);
    }

    cleanup();
}
