#include "logger.hpp"
#include "webserv.hpp"

int main()
{
    File::_build_mime_table();
    ws::log.init();
    Webserv webserv;

    webserv.initialize();
    webserv.eventLoop();
}
