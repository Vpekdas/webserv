#include "cgi.hpp"
#include <unistd.h>

CGI::CGI()
{
}

CGI::CGI(std::string path) : m_path(path), m_pid(-1)
{
}

// https://stackoverflow.com/questions/7047426/call-php-from-virtual-custom-web-server

std::string CGI::process(std::string filepath)
{
    pipe(m_pipefds);

    m_pid = fork();
    if (m_pid == -1)
        return ""; // TODO: Returns an error (maybe 500 ?)

    if (m_pid == 0)
    {
        dup2(m_pipefds[1], STDOUT_FILENO);

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
        execve(m_path.c_str(), (char **)argv, (char **)envp);
        return std::string();
    }
    else
    {
        ssize_t n;
        std::string str;
        char buf[1024];

        close(m_pipefds[1]);

        while ((n = read(m_pipefds[0], buf, 1024)) > 0)
            str.append(buf, n);

        return str;
    }
}
