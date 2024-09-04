#include "webserv.hpp"
#include <cstring>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>

Webserv::Webserv() : m_router("www")
{
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

    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_addr.s_addr = INADDR_ANY;
    m_sockAddr.sin_port = htons(9999);

    // Ensure the server can accept incoming connections by binding the socket to
    // the specified IP address and port. This step is crucial for the server to
    // listen for and accept client requests on the designated network interface.
    if (bind(m_sockFd, (struct sockaddr *)&m_sockAddr, sizeof(sockaddr)) == FAILURE)
    {
        std::cerr << NRED << strerror(errno) << RED << ": bind() failed." << RESET << std::endl;
        return FAILURE;
    }

    // Prepare the socket to accept incoming connection requests by setting it to
    // a listening state. This is essential for the server to handle multiple
    // client connections concurrently.
    if (listen(m_sockFd, 42) == FAILURE)
    {
        std::cerr << NRED << strerror(errno) << RED << ": listen() failed." << RESET << std::endl;
        return FAILURE;
    }
    return SUCCESS;
}

Result<Connection, int> Webserv::acceptConnection()
{
    socklen_t addrLen = sizeof(struct sockaddr_in);
    struct sockaddr_in addr = {};

    int conn = accept(m_sockFd, (struct sockaddr *)&addr, &addrLen);
    if (conn == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": accept() failed." << RESET << std::endl;
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = conn;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, conn, &event) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
        close(m_epollFd);
    }

    return Connection(conn, addr, event);
}

void Webserv::eventLoop()
{
    int eventCount = 0;
    struct epoll_event events[MAX_EVENTS];

    struct epoll_event socket_event;
    socket_event.events = EPOLLIN;
    socket_event.data.fd = m_sockFd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_sockFd, &socket_event) == -1)
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;

    while (1)
    {
        std::cout << YELLOW << "Polling for input..." << RESET << std::endl;

        eventCount = epoll_wait(m_epollFd, events, MAX_EVENTS, -1);
        std::cout << GREEN << eventCount << " ready events." << RESET << std::endl;

        for (int i = 0; i < eventCount; i++)
        {
            if (events[i].data.fd == m_sockFd)
            {
                Connection conn = acceptConnection().unwrap();
                m_connections[conn.fd()] = conn;
                continue;
            }

            // Check if the current event indicates that there is data to read on the
            // file descriptor.
            // This is crucial for processing incoming data from the client, ensuring
            // the server can handle read operations when data is available.
            if ((events[i].events & EPOLLIN) == EPOLLIN)
            {
                std::cout << CYAN << "Reading file descriptor: " << events[i].data.fd << RESET << std::endl;

                int n;
                char buf[READ_SIZE];

                Connection& conn = m_connections[events[i].data.fd];
                std::string& req_str = conn.getReqStr();

                n = recv(events[i].data.fd, buf, READ_SIZE, 0);
                req_str.append(buf, n);

                if (n == -1)
                    std::cerr << NRED << strerror(errno) << RED << ": recv() failed." << RESET << std::endl;

                // Modify the event to monitor the file descriptor for outgoing data.
                // This is necessary to prepare the server to send a response back to
                // the client after reading the incoming data.
                struct epoll_event event;
                event.events = EPOLLOUT | EPOLLET;
                event.data.fd = events[i].data.fd;

                if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, events[i].data.fd, &event) == -1)
                {
                    std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
                    close(events[i].data.fd);
                }
            }
            // Check if the current event indicates that the file descriptor is ready
            // for writing. This is essential for sending data to the client, ensuring
            // the server can handle write operations when the socket is ready.
            else if ((events[i].events & EPOLLOUT) == EPOLLOUT)
            {
                Connection& conn = m_connections[events[i].data.fd];

                Result<Request, int> res2 = Request::parse(conn.getReqStr());

                if (res2.is_err())
                {
                    std::cerr << NRED << "empty request from " << conn.fd() << RESET << "\n";
                    close_connection(conn);
                    continue;
                }

                Request req = res2.unwrap(); // FIXME: `req_str` is empty and crash after a connection is closed
                conn.getReqStr().clear();
                Result<File *, HttpStatus> res = m_router.route(req.path());

                Response response;

                if (res.is_err())
                    response = Response::httpcat(res.unwrap_err());
                else
                    response = Response::ok(200, res.unwrap());

                std::cout << response.encode_header() << "\n";

                if (req.is_keep_alive())
                    response.add_param("Connection", "keep-alive");

                // response.add_param("X-Content-Type-Options", "nosniff");

                response.send(events[i].data.fd);

                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = events[i].data.fd;

                // Close the connection if the client either close the connection or don't want to keep
                // it alive.
                if (!req.is_keep_alive() || req.is_closed())
                    close_connection(conn);
                else
                    keep_alive(conn);
            }
        }
    }
    close(m_epollFd);
    close(m_sockFd);
}

void Webserv::close_connection(Connection& conn)
{
    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, conn.fd(), NULL) == -1)
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
    close(conn.fd());
    m_connections.erase(m_connections.find(conn.fd()));
}

void Webserv::keep_alive(Connection& conn)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = conn.fd();

    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, conn.fd(), &event) == -1)
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
}
