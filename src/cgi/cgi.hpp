#pragma once

/*
    A simple CGI which communicates with `STDINT` and `STDOUT`.
 */
#include <fcntl.h>
#include <string>

#include "http/status.hpp"
#include "result.hpp"

class CGI
{
public:
    CGI();
    CGI(std::string path);

    Result<std::string, HttpStatus> process(std::string filepath);

private:
    /* The CGI to execute. */
    std::string m_path;
    /* PID of the children process that holds the CGI. */
    pid_t m_pid;

    int m_pipefds[2];
};
