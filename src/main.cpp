#include "webserv.hpp"

int main()
{
    File::_build_mime_table();
    Webserv webserv;

    webserv.initialize();
    webserv.eventLoop();
}
