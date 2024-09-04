#include "cgi.hpp"
#include "result.hpp"

#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

CGI::CGI()
{
}

CGI::CGI(std::string path) : m_path(path), m_pid(-1)
{
}

// https://stackoverflow.com/questions/7047426/call-php-from-virtual-custom-web-server

Result<std::string, HttpStatus> CGI::process(std::string filepath)
{
    if (pipe(m_pipefds) == -1)
        return Err<std::string, HttpStatus>(500);

    m_pid = fork();
    if (m_pid == -1)
    {
        close(m_pipefds[0]);
        close(m_pipefds[1]);
        return Err<std::string, HttpStatus>(500);
    }

    if (m_pid == 0)
    {
        if (dup2(m_pipefds[1], STDOUT_FILENO) == -1)
        {
            close(m_pipefds[0]);
            close(m_pipefds[1]);
            exit(1);
        }

        std::string filepath_env = "SCRIPT_FILENAME=" + filepath;

        const char *argv[] = {NULL};
        // clang-format off
        const char *envp[] = {
            "GATEWAY_INTERFACE=GCI/1.1",
            filepath_env.c_str(),
            "QUERY_STRING=\"\"",
            "REQUEST_METHOD=\"GET\"",
            "REDIRECT_STATUS=200",
            NULL
        };
        // clang-format on

        if (execve(m_path.c_str(), (char **)argv, (char **)envp) == -1)
        {
            close(m_pipefds[0]);
            close(m_pipefds[1]);
        }

        exit(1);
    }
    else
    {
        // TODO: Why wont this work ?

        // int stat_loc;
        // pid_t pid;
        // while ((pid = waitpid(m_pid, &stat_loc, WNOHANG)) >= 0)
        //     ;

        // if (pid == -1 || WEXITSTATUS(stat_loc) != 0)
        //     return Err<std::string, HttpStatus>(500);

        ssize_t n;
        std::string str;
        char buf[1024];

        close(m_pipefds[1]);

        while ((n = read(m_pipefds[0], buf, 1024)) > 0)
            str.append(buf, n);

        close(m_pipefds[0]);
        close(m_pipefds[1]);

        if (n == -1)
            return Err<std::string, HttpStatus>(500);

        return str;
    }
}
