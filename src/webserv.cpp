#include "webserv.hpp"

Webserv::Webserv()
{
    std::cout << YELLOW << "🛠️ Default WebServ Constructor called 🛠️" << RESET << std::endl;
}

Webserv::~Webserv()
{
    std::cout << RED << "🧨 WebServ Destructor called 🧨" << RESET << std::endl;
}

Webserv::Webserv(const Webserv& other)
{
    (void)other;
    std::cout << YELLOW << "🖨️ WebServ Copy Constructor called 🖨️" << RESET << std::endl;
}

Webserv& Webserv::operator=(const Webserv& other)
{
    // Check for self-assignment
    if (this != &other)
    {
    }
    std::cout << YELLOW << "📞 WebServ Copy Assignment Operator called 📞" << RESET << std::endl;
    return *this;
}

int Webserv::getEpollFd() const
{
    return m_epollFd;
}

int Webserv::getSockFd() const
{
    return m_sockFd;
}

int Webserv::initialize()
{
    // Initialize epoll to efficiently manage multiple file descriptors for I/O
    // events.
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": epoll_create1() failed." << RESET << std::endl;
        return FAILURE;
    }

    // Create a socket for network communication using IPv4 and TCP.
    // This socket will be used to listen for incoming connections.
    m_sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockFd == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": socket() failed." << RESET << std::endl;
        return FAILURE;
    }

    // By doing this, we can use the same port after relaunching the web server.
    // Without it, the port remains in use for a while.
    int b = true;
    if (setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEPORT, &b, sizeof(int)) == FAILURE)
    {
        std::cerr << NRED << strerror(errno) << RED << ": setsockopt() failed." << RESET << std::endl;
        return FAILURE;
    }

    return SUCCESS;
}
