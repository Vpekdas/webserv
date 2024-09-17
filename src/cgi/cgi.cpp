#include "cgi.hpp"
#include "http/request.hpp"
#include "logger.hpp"
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

Result<std::string, HttpStatus> CGI::process(std::string filepath, Request& req, int timeout, std::string& req_str)
{
    if (pipe(m_stdout) == -1)
        return HttpStatus(500);
    if (pipe(m_stdin) == -1)
        return HttpStatus(500);

    m_start_time = time();

    m_pid = fork();
    if (m_pid == -1)
    {
        close(m_stdout[0]);
        close(m_stdout[1]);
        close(m_stdin[0]);
        close(m_stdin[1]);
        return HttpStatus(500);
    }

    if (m_pid == 0)
    {
        if (dup2(m_stdout[1], STDOUT_FILENO) == -1)
        {
            close(m_stdout[0]);
            close(m_stdout[1]);
            close(m_stdin[0]);
            close(m_stdin[1]);
            exit(1);
        }

        if (dup2(m_stdin[0], STDIN_FILENO) == -1)
        {
            close(m_stdout[0]);
            close(m_stdout[1]);
            close(m_stdin[0]);
            close(m_stdin[1]);
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

        std::vector<std::string> envp;
        envp.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envp.push_back("REDIRECT_STATUS=200");

        // NOTE: May be specific to php but not 100% sure
        // if (m_path.rfind("/php-cgi") == m_path.size() - 8)
        // {
            std::string script_filename = "SCRIPT_FILENAME=" + filepath;
            envp.push_back(script_filename.c_str());
        // }

        envp.push_back("REQUEST_METHOD=" + std::string(strmethod(req.method())));
        envp.push_back("HTTP_COOKIE=" + req.cookies());
        envp.push_back("HTTP_USER_AGENT=" + req.user_agent());

        if (req.method() == POST)
        {
            envp.push_back("CONTENT_LENGTH=" + req.get_param("Content-Length"));
            envp.push_back("CONTENT_TYPE=" + req.get_param("Content-Type"));
        }
        else
        {
            envp.push_back("QUERY_STRING=" + req.args_str());
        }
        // clang-format on

        char **env2 = (char **)std::calloc(sizeof(char *), envp.size() + 1);
        if (!env2)
        {
            free(env2);
            exit(1);
        }

        for (size_t i = 0; i < envp.size(); i++)
        {
            env2[i] = strdup(envp[i].c_str());
            if (!env2[i])
            {
                for (size_t j = 0; j < i; j++)
                    free(env2[j]);
                free(env2);
                exit(1);
            }
        }
        env2[envp.size()] = NULL;

        // TODO: maybe use chdir here.

        if (execve(m_path.c_str(), (char **)argv, (char **)env2) == -1)
        {
            close(m_stdout[0]);
            close(m_stdout[1]);
            close(m_stdin[0]);
            close(m_stdin[1]);
            for (size_t i = 0; env2[i]; i++)
                free(env2[i]);
            free(env2);
        }
        exit(1);
    }
    else
    {
        int stat_loc;
        ssize_t n;

        if (req.method() == POST && !req_str.empty())
            n = write(m_stdin[1], req_str.c_str(), req_str.size());
        close(m_stdin[1]);

        while (waitpid(m_pid, &stat_loc, WNOHANG) == 0)
        {
            if (timeout > 0 && time() - m_start_time > timeout)
            {
                kill(m_pid, SIGQUIT);
                ws::log << ws::warn << "CGI `" << m_path << "` timed out\n";
                return HttpStatus(500);
            }
        }

        std::string str;
        char buf[1024];

        close(m_stdout[1]);

        while ((n = read(m_stdout[0], buf, 1024)) > 0)
            str.append(buf, n);

        close(m_stdout[0]);
        close(m_stdin[1]);

        std::cout << str << "\n";

        if (!WIFEXITED(stat_loc) || WEXITSTATUS(stat_loc) != 0 || n == -1)
        {
            // std::cout << str << "\n";
            return HttpStatus(500);
        }

        return str;
    }
}
