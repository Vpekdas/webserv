#include "webserv.hpp"
#include "config/config.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "server.hpp"
#include <cstring>
#include <iostream>

Webserv::Webserv() : m_running(true)
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

void Webserv::quit()
{
    m_running = false;
}

int Webserv::initialize(std::string config_path)
{
    if (access(config_path.c_str(), F_OK | R_OK) == -1)
    {
        ws::log << ws::err << "Invalid config file `" << config_path << "`: " << strerror(errno) << "\n";
        return FAILURE;
    }

    Result<int, ConfigError> res = m_config.load_from_file(config_path);
    if (res.is_err())
    {
        ws::log << ws::err << "Configuration error:\n";
        res.unwrap_err().print(ws::log);
        return FAILURE;
    }

    m_router = Router(m_config.servers()[0]);

    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1)
    {
        ws::log << ws::err << ": epoll_create1() failed: " << strerror(errno) << RESET << "\n";
        return FAILURE;
    }

    m_sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockFd == -1)
    {
        ws::log << ws::err << RED << ": socket() failed: " << strerror(errno) << RESET << "\n";
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
    ws::log << ws::info << "Listening on port " << ntohs(m_sockAddr.sin_port) << "\n";

    if (bind(m_sockFd, (struct sockaddr *)&m_sockAddr, sizeof(sockaddr)) == FAILURE)
    {
        std::cerr << NRED << strerror(errno) << RED << ": bind() failed." << RESET << std::endl;
        return FAILURE;
    }

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
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    event.data.fd = conn;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, conn, &event) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
        close(m_epollFd);
    }
    return Connection(conn, addr);
}

void Webserv::eventLoop()
{
    int eventCount = 0;
    struct epoll_event events[MAX_EVENTS];

    // ServerConfig serverConfig;
    // Server server(serverConfig);

    struct epoll_event socket_event;
    socket_event.events = EPOLLIN;
    socket_event.data.fd = m_sockFd;

    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_sockFd, &socket_event) == -1)
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;

    // if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, server.sock_fd(), &socket_event) == -1)
    //     std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;

    while (m_running)
    {
        eventCount = epoll_wait(m_epollFd, events, MAX_EVENTS, -1);
        for (int i = 0; i < eventCount; i++)
        {
            if (events[i].data.fd == m_sockFd)
            {
                Connection conn = acceptConnection().unwrap();
                if (m_connections.count(conn.fd()) > 0)
                    ws::log << ws::dbg << "Connection " << conn.fd() << " already in map\n";
                m_connections[conn.fd()] = conn;
                continue;
            }

            if ((events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)))
            {
                Connection& conn = m_connections[events[i].data.fd];
                close_connection(conn);
                continue;
            }

            else if (events[i].events & EPOLLIN)
            {
                int n;
                char buf[READ_SIZE + 1];

                Connection& conn = m_connections[events[i].data.fd];
                std::string& req_str = conn.getReqStr();

                n = recv(events[i].data.fd, buf, READ_SIZE, 0);
                buf[n] = '\0';

                if (n == -1)
                {
                    ws::log << ws::err << "recv() failed: " << strerror(errno) << "\n";
                    close_connection(conn);
                    continue;
                }

                if (!conn.waiting_for_body())
                    req_str.append(buf, n);

                // We reach the end of the header, we can now parse it and check the `Content-Length` and other
                // parameters.
                if (!conn.waiting_for_body() && std::strstr(buf, SEP SEP))
                {
                    Request req = Request::parse(req_str).unwrap();
                    conn.set_last_request(req);

                    std::string leftovers = req_str.substr(req.header_size());
                    if (!leftovers.empty())
                    {
                        // TODO: Process the data.
                        conn.set_bytes_read(leftovers.size());
                    }

                    if (req.content_length() == (size_t)-1)
                    {
                        // TODO: Missing `Content-Length`, should return an error or threat it as `0` ?
                    }

                    req_str.clear();
                }
                else if (conn.waiting_for_body())
                {
                    conn.set_bytes_read(conn.bytes_read() + n);
                    // TODO: Process the data.
                }

                if (n < READ_SIZE || (n == READ_SIZE && conn.bytes_read() > conn.last_request().content_length()))
                {
                    struct epoll_event event;
                    event.events = EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
                    event.data.fd = events[i].data.fd;

                    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, events[i].data.fd, &event) == -1)
                    {
                        ws::log << ws::err << "epoll_ctrl() failed: " << strerror(errno) << "\n";
                        close(events[i].data.fd);
                        continue;
                    }
                }
            }

            else if ((events[i].events & EPOLLOUT))
            {
                Connection& conn = m_connections[events[i].data.fd];

                Request req = conn.last_request();
                Response response;

                if (conn.bytes_read() > req.content_length())
                {
                    response = Response::httpcat(413); // Payload too large
                }
                else
                {
                    Result<Response, HttpStatus> res = m_router.route(req);

                    if (res.is_err())
                        response = Response::httpcat(res.unwrap_err());
                    else
                        response = res.unwrap();
                }

                if (req.is_keep_alive())
                    response.add_param("Connection", "keep-alive");

                if (!response.status().is_error())
                {
                    ws::log << ws::info << "GET `" << req.path() << "` -> " << NGREEN << response.status().code() << " "
                            << response.status() << RESET << "\n";
                }
                else
                    ws::log << ws::info << "GET `" << req.path() << "` -> " << NRED << response.status().code() << " "
                            << response.status() << RESET << "\n";

                response.send(events[i].data.fd);

                struct epoll_event event;
                event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
                event.data.fd = events[i].data.fd;

                if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, events[i].data.fd, &event) == -1)
                {
                    ws::log << ws::err << "epoll_ctrl(EPOLL_CTL_MOD) failed: " << strerror(errno) << "\n";
                    close(events[i].data.fd);
                    continue;
                }

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
        ws::log << ws::err << "epoll_ctl(EPOLL_CTL_DEL) failed: " << strerror(errno) << "\n";
    close(conn.fd());
    m_connections.erase(m_connections.find(conn.fd()));
}

void Webserv::keep_alive(Connection& conn)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    event.data.fd = conn.fd();

    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, conn.fd(), &event) == -1)
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
}
