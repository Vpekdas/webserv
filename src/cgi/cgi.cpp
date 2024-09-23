#include "cgi.hpp"
#include "http/request.hpp"
#include "http/status.hpp"
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

Result<Response, HttpStatus> CGI::process(std::string filepath, Request& req, int timeout, std::string& req_str)
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
        std::string parent = filepath.substr(0, filepath.rfind('/'));
        std::string filename = filepath = filepath.substr(filepath.rfind('/') + 1);

        const char *argv[] = {
            m_path.c_str(),
            filename.c_str(),
            NULL
        };

        // RFC describing Common Gateway Interface
        // https://www.ietf.org/rfc/rfc3875.txt

        std::vector<std::string> envp;
        envp.push_back("GATEWAY_INTERFACE=CGI/1.1");

        // NOTE: This is required by `php-cgi` but not part of the CGI standard.
        envp.push_back("REDIRECT_STATUS=200");

        // NOTE: May be specific to php but not 100% sure
        envp.push_back("SCRIPT_FILENAME=" + filename);

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
            exit(1);

        for (size_t i = 0; i < envp.size(); i++)
        {
            env2[i] = strdup(envp[i].c_str());
            if (!env2[i])
            {
                for (size_t j = 0; j < i; j++)
                    free(env2[j]);
                free(env2);
                close(m_stdout[0]);
                close(m_stdout[1]);
                close(m_stdin[0]);
                close(m_stdin[1]);

                g_webserv.closeFds();
                exit(1);
            }
        }
        env2[envp.size()] = NULL;

        close(m_stdout[0]);
        close(m_stdout[1]);
        close(m_stdin[0]);
        close(m_stdin[1]);

        g_webserv.closeFds();

        if (chdir(parent.c_str()) == -1)
        {
            for (size_t i = 0; env2[i]; i++)
                free(env2[i]);
            free(env2);
        }

        if (execve(m_path.c_str(), (char **)argv, (char **)env2) == -1)
        {
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
        {
            n = write(m_stdin[1], req_str.c_str(), req_str.size());

            if (n == 0 || n == -1)
            {
                kill(m_pid, SIGQUIT);
                close(m_stdout[1]);
                close(m_stdout[0]);
                close(m_stdin[0]);
                close(m_stdin[1]);
                return HttpStatus(500);
            }
        }

        close(m_stdin[1]);

        while (waitpid(m_pid, &stat_loc, WNOHANG) == 0)
        {
            if (timeout > 0 && time() - m_start_time > timeout)
            {
                kill(m_pid, SIGQUIT);
                ws::log << ws::warn << "CGI `" << m_path << "` timed out\n";
                close(m_stdout[1]);
                close(m_stdout[0]);
                close(m_stdin[0]);
                return HttpStatus(500);
            }
        }

        std::string str;
        char buf[1024];

        close(m_stdout[1]);

        while ((n = read(m_stdout[0], buf, 1024)) > 0)
            str.append(buf, n);

        close(m_stdout[0]);
        close(m_stdin[0]);

        if (!WIFEXITED(stat_loc) || WEXITSTATUS(stat_loc) != 0 || n == -1)
            return HttpStatus(500);

        size_t pos = str.find(SEP SEP);

        if (pos == std::string::npos)
            return HttpStatus(500);

        size_t headerSize = pos + 2;
        Response response = Response::from_cgi(200, str.substr(0, headerSize));

        HttpStatus status = 200;

        if (response.has_param("Location"))
        {
            status = 307;
        }

        return Response::from_cgi(status, str);
    }
}
