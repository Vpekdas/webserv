#include <cstring>
#include <netinet/in.h>

#include "logger.hpp"
#include "server.hpp"

Server::Server() : m_sock_fd(-1)
{
}

Server::Server(struct sockaddr_in addr) : m_addr(addr)
{
    m_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock_fd == -1)
        ws::log << ws::err << "socket() failed: " << strerror(errno) << "\n";

    // By doing this, we can use the same port after relaunching the web server.
    // Without it, the port remains in use for a while.
    int b = true;
    if (setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEPORT, &b, sizeof(int)) == -1)
        ws::log << ws::err << "setsockopt() failed: " << strerror(errno) << "\n";

    if (bind(m_sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
        ws::log << ws::err << "bind() failed: " << strerror(errno) << "\n";

    if (listen(m_sock_fd, 42) == -1)
        ws::log << ws::err << "listen() failed: " << strerror(errno) << "\n";
}

Server::~Server()
{
}

int Server::sock_fd()
{
    return m_sock_fd;
}
