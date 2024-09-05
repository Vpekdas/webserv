#include "logger.hpp"
#include "webserv.hpp"

int main(int argc, char *argv[])
{
    ws::log.init();

    if (argc != 2)
    {
        ws::log << ws::err << "Missing configuration file\n";
        return 1;
    }

    File::_build_mime_table();
    Webserv webserv;

    if (webserv.initialize(argv[1]) != 0)
        return 1;
    webserv.eventLoop();
}
