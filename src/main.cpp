#include "logger.hpp"
#include "webserv.hpp"
#include <csignal>

static Webserv webserv;

void sigpipe(int signum)
{
    (void)signum;
    ws::log << ws::dbg << "SIGPIPE received\n";
}

void signal_handler(int signum)
{
    (void)signum;
    ws::log << ws::info << "Shutdown...\n";
    webserv.quit();
}

int main(int argc, char *argv[])
{
    ws::log.init();

    if (argc != 2)
    {
        ws::log << ws::err << "Missing configuration file\n";
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGPIPE, sigpipe);

    File::_build_mime_table();

    if (webserv.initialize(argv[1]) != 0)
        return 1;
    webserv.eventLoop();
}
