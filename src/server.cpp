#include "server.hpp"
#include "webserv.hpp"

Server::Server(ServerConfig& config) : m_config(config)
{
    m_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock_fd == -1)
        ws::log << ws::err << RED << ": socket() failed: " << strerror(errno) << RESET << "\n";

    // By doing this, we can use the same port after relaunching the web server.
    // Without it, the port remains in use for a while.
    int b = true;
    if (setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEPORT, &b, sizeof(int)) == FAILURE)
        std::cerr << NRED << strerror(errno) << RED << ": setsockopt() failed." << RESET << std::endl;

    ws::log << ws::info << "Listening on port " << ntohs(config.listen_addr().sin_port) << "\n";

    if (bind(m_sock_fd, (struct sockaddr *)&m_config.listen_addr(), sizeof(sockaddr)) == FAILURE)
        std::cerr << NRED << strerror(errno) << RED << ": bind() failed." << RESET << std::endl;

    if (listen(m_sock_fd, 42) == FAILURE)
        std::cerr << NRED << strerror(errno) << RED << ": listen() failed." << RESET << std::endl;
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