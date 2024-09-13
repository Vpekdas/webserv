#include "cgi.hpp"
#include "http/request.hpp"
#include "result.hpp"
#include "webserv.hpp"

#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

CGI::CGI()
{
}

CGI::CGI(std::string path) : m_path(path), m_pid(-1)
{
}

// https://stackoverflow.com/questions/7047426/call-php-from-virtual-custom-web-server

Result<std::string, HttpStatus> CGI::process(std::string filepath, Request& req)
{
    if (pipe(m_pipefds) == -1)
        return Err<std::string, HttpStatus>(500);

    m_start_time = time();

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

        std::string script_filename = "SCRIPT_FILENAME=" + filepath;

        // clang-format off
        const char *argv[] = {
            m_path.c_str(),
            filepath.c_str(),
            NULL
        };

        // RFC describing Common Gateway Interface
        // https://www.ietf.org/rfc/rfc3875.txt

        std::vector<const char *> envp;
        envp.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envp.push_back("REDIRECT_STATUS=200");

        std::string request_method = "REQUEST_METHOD=" + std::string(strmethod(req.method()));
        std::string http_cookie = "HTTP_COOKIE=" + req.cookies();
        std::string http_user_agent = "HTTP_USER_AGENT=" + req.user_agent();

        envp.push_back(strdup(request_method.c_str()));
        envp.push_back(strdup(http_cookie.c_str()));
        envp.push_back(strdup(http_user_agent.c_str()));

        if (req.method() == POST)
        {
            std::string content_length = "CONTENT_LENGTH=" + req.get_param("Content-Length");
            std::string content_type = "CONTENT_TYPE=" + req.get_param("Content-Type");
            envp.push_back(strdup(content_length.c_str()));
            envp.push_back(strdup(content_type.c_str()));
        }
        else
        {
            std::string query_string = "QUERY_STRING=" + req.args_str();
            envp.push_back(strdup(query_string.c_str()));
        }

        envp.push_back(NULL);
        // clang-format on

        // TODO: maybe use chdir here.

        // std::cerr << CYAN << "PATH" << RESET << std::endl;
        // std::cerr << CYAN << m_path << RESET << std::endl;

        // std::cerr << CYAN << "ARGV" << RESET << std::endl;
        // for (int i = 0; argv[i]; i++)
        // {
        //     std::cerr << CYAN << i << ": " << argv[i] << RESET << std::endl;
        // }

        // std::cerr << CYAN << "ENVP" << RESET << std::endl;
        // for (int i = 0; envp[i]; i++)
        // {
        //     std::cerr << CYAN << i << ": " << envp[i] << RESET << std::endl;
        // }

        if (execve(m_path.c_str(), (char **)argv, (char **)envp.data()) == -1)
        {
            close(m_pipefds[0]);
            close(m_pipefds[1]);
        }
        exit(1);
    }
    else
    {
        // TODO: Why wont this work ?
        int stat_loc;
        pid_t result;

        while ((result = wait(&stat_loc)) > 0)
            ;

        if (WIFEXITED(stat_loc))
        {
            int exit_status = WEXITSTATUS(stat_loc);
            // std::cout << "Exit status: " << exit_status << "\n";
            if (exit_status != 0)
                return Err<std::string, HttpStatus>(500);
        }
        else
        {
            // std::cerr << "Child process did not terminate normally\n";
            return Err<std::string, HttpStatus>(500);
        }

        ssize_t n;
        std::string str;
        char buf[1024];

        close(m_pipefds[1]);

        while ((n = read(m_pipefds[0], buf, 1024)) > 0)
            str.append(buf, n);

        close(m_pipefds[0]);

        if (n == -1)
            return Err<std::string, HttpStatus>(500);

        return str;
    }
}
