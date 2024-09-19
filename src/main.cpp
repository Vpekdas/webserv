#include "logger.hpp"
#include "webserv.hpp"
#include <csignal>

Webserv g_webserv;
char **g_envp;

void sigpipe(int signum)
{
    (void)signum;
    ws::log << ws::dbg << "SIGPIPE received\n";
}

void signal_handler(int signum)
{
    (void)signum;
    ws::log << ws::info << "Shutdown...\n";
    g_webserv.quit();
}

int main(int argc, char *argv[], char *envp[])
{
    ws::log.init();

    if (argc != 2)
    {
        ws::log << ws::err << "Incorrect number of arguments.\n";
        return 1;
    }

    g_envp = envp;

    signal(SIGINT, signal_handler);
    signal(SIGPIPE, sigpipe);

    File::_build_mime_table();
    Response::_build_themes();

    if (g_webserv.initialize(argv[1]) != 0)
        return 1;
    g_webserv.eventLoop();
}
