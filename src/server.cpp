#include "server.hpp"
#include "webserv.hpp"
#include <netinet/in.h>

Server::Server() : m_sock_fd(-1)
{
}

Server::Server(ServerConfig config) : m_config(config), m_router(config)
{
    m_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock_fd == -1)
        ws::log << ws::err << "socket() failed: " << strerror(errno) << "\n";

    // By doing this, we can use the same port after relaunching the web server.
    // Without it, the port remains in use for a while.
    int b = true;
    if (setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEPORT, &b, sizeof(int)) == FAILURE)
        std::cerr << NRED << strerror(errno) << RED << ": setsockopt() failed." << RESET << std::endl;

    // struct sockaddr_in addr;
    // addr.sin_family = AF_INET;
    // addr.sin_addr.s_addr = INADDR_ANY;
    // addr.sin_port = htons(9999);

    if (bind(m_sock_fd, (struct sockaddr *)&config.listen_addr(), sizeof(struct sockaddr_in)) == FAILURE)
        std::cerr << NRED << strerror(errno) << RED << ": bind() failed." << RESET << std::endl;

    if (listen(m_sock_fd, 42) == FAILURE)
        std::cerr << NRED << strerror(errno) << RED << ": listen() failed." << RESET << std::endl;

    ws::log << ws::info << "Listening on port " << ntohs(config.listen_addr().sin_port) << "\n";
}

Server::~Server()
{
}

int Server::sock_fd()
{
    return m_sock_fd;
}

ServerConfig& Server::config()
{
    return m_config;
}

Router& Server::router()
{
    return m_router;
}
