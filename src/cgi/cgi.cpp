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

Result<std::string, HttpStatus> CGI::process(std::string filepath, Request& req, int timeout)
{
    if (pipe(m_pipefds) == -1)
        return HttpStatus(500);

    m_start_time = time();

    m_pid = fork();
    if (m_pid == -1)
    {
        close(m_pipefds[0]);
        close(m_pipefds[1]);
        return HttpStatus(500);
    }

    if (m_pid == 0)
    {
        if (dup2(m_pipefds[1], STDOUT_FILENO) == -1)
        {
            close(m_pipefds[0]);
            close(m_pipefds[1]);
            exit(1);
        }

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

        // NOTE: May be specific to php but not 100% sure
        // if (m_path.rfind("/php-cgi") == m_path.size() - 8)
        // {
            std::string script_filename = "SCRIPT_FILENAME=" + filepath;
            envp.push_back(strdup(script_filename.c_str()));
        // }

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

        if (execve(m_path.c_str(), (char **)argv, (char **)envp.data()) == -1)
        {
            close(m_pipefds[0]);
            close(m_pipefds[1]);
        }
        exit(1);
    }
    else
    {
        int stat_loc;

        while (waitpid(m_pid, &stat_loc, WNOHANG) == 0)
        {
            if (timeout > 0 && time() - m_start_time > timeout)
            {
                kill(m_pid, SIGQUIT);
                return HttpStatus(500);
            }
        }

        ssize_t n;
        std::string str;
        char buf[1024];

        close(m_pipefds[1]);

        while ((n = read(m_pipefds[0], buf, 1024)) > 0)
            str.append(buf, n);

        close(m_pipefds[0]);

        if (!WIFEXITED(stat_loc) || WEXITSTATUS(stat_loc) != 0 || n == -1)
        {
            std::cout << str << "\n";
            return Err<std::string, HttpStatus>(500);
        }

        return str;
    }
}
