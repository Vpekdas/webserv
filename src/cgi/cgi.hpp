#pragma once

#include <fcntl.h>
#include <string>

#include "http/request.hpp"
#include "http/status.hpp"
#include "result.hpp"

class CGI
{
public:
    CGI();
    CGI(std::string path);

    Result<std::string, HttpStatus> process(std::string filepath, Request& req, int timeout);

private:
    /* The CGI to execute. */
    std::string m_path;
    /* PID of the children process that holds the CGI. */
    pid_t m_pid;

    int m_pipefds[2];
    int64_t m_start_time;
};
