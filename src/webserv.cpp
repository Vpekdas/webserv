#include "webserv.hpp"
#include "config/config.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "logger.hpp"
#include "server.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <unistd.h>

Webserv::Webserv() : m_running(true)
{
}

int Webserv::getEpollFd() const
{
    return m_epollFd;
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
        return -1;
    }

    Result<int, ConfigError> res = m_config.load_from_file(config_path);
    if (res.is_err())
    {
        ws::log << ws::err << "Configuration error:\n";
        res.unwrap_err().print(ws::log);
        return -1;
    }

    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1)
    {
        ws::log << ws::err << ": epoll_create1() failed: " << strerror(errno) << RESET << "\n";
        return -1;
    }

    return 0;
}

Result<Connection, int> Webserv::acceptConnection(int sock_fd)
{
    socklen_t addrLen = sizeof(struct sockaddr_in);
    struct sockaddr_in addr = {};

    int conn = accept(sock_fd, (struct sockaddr *)&addr, &addrLen);
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
    return Connection(conn, sock_fd, addr);
}

void Webserv::eventLoop()
{
    for (size_t i = 0; i < m_config.servers().size(); i++)
    {
        ServerConfig& config = m_config.servers()[i];
        std::string host = config.server_name();

        if (has_server(config.listen_addr()))
        {
            Server& server = get_server(config.listen_addr());
            server.add_host(host, config);
        }
        else
        {
            Server server(config.listen_addr());
            server.add_host(host, config);

            struct epoll_event socket_event;
            socket_event.events = EPOLLIN;
            socket_event.data.fd = server.sock_fd();

            if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, server.sock_fd(), &socket_event) == -1)
            {
                std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
                continue;
            }

            m_servers[server.sock_fd()] = server;
        }

        ws::log << ws::info << "Host " BIWHITE << host << RESET " listening on " << config.listen_addr() << "\n";
    }

    while (m_running)
    {
        poll_events();
    }

    // Close all servers currently listening.
    // for (size_t i = 0; i < m_servers.size(); i++)
    //     close(m_servers[i].sock_fd());

    // Close all remaining connections.
    // for (size_t i = 0; i < m_connections.size(); i++)
    //     close(m_connections[i].fd());

    close(m_epollFd);
}

void Webserv::poll_events()
{
    int eventCount = 0;
    struct epoll_event events[MAX_EVENTS];

    eventCount = epoll_wait(m_epollFd, events, MAX_EVENTS, -1);
    for (int i = 0; i < eventCount; i++)
    {
        if (m_servers.count(events[i].data.fd) > 0)
        {
            Connection conn = acceptConnection(events[i].data.fd).unwrap();
            conn.set_last_event(time());
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
            std::string& req_str = conn.req_str();

            n = recv(events[i].data.fd, buf, READ_SIZE, 0);
            buf[n] = '\0';

            conn.set_last_event(time());

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

                std::string host_str = req.get_param("Host").substr(0, req.get_param("Host").find(':'));
                Host& host = m_servers[conn.sock_fd()].default_host();

                std::string leftovers = req_str.substr(req.header_size());
                req_str.clear();
                if (!leftovers.empty())
                {
                    // TODO: Process the data.
                    conn.set_bytes_read(leftovers.size());
                    conn.set_req_str(leftovers);
                }

                if (!req.has_param("Content-Length") || leftovers.size() > host.config().max_content_length() ||
                    leftovers.size() > req.content_length())
                {
                    conn.set_epollout(m_epollFd);
                    continue;
                }
            }
            else if (conn.waiting_for_body())
            {
                Request& req = conn.last_request();
                std::string host_str = req.get_param("Host").substr(0, req.get_param("Host").find(':'));
                Host& host = m_servers[conn.sock_fd()].default_host();

                // TODO: Process the data.
                conn.set_bytes_read(conn.bytes_read() + n);
                conn.req_str().append(buf, n);

                if (conn.bytes_read() > host.config().max_content_length() || conn.bytes_read() > req.content_length())
                    conn.set_epollout(m_epollFd);
            }

            if (n < READ_SIZE)
                conn.set_epollout(m_epollFd);
        }

        else if ((events[i].events & EPOLLOUT))
        {
            Connection& conn = m_connections[events[i].data.fd];
            Request req = conn.last_request();

            std::string host_str = req.get_param("Host").substr(0, req.get_param("Host").find(':'));
            Host& host = m_servers[conn.sock_fd()].default_host();

            if (m_servers[conn.sock_fd()].has_host(host_str))
                host = m_servers[conn.sock_fd()].host(host_str);

            if (req.method() == POST && req.content_type() == "application/x-www-form-urlencoded")
                req.set_args(conn.req_str());

            Response response;
            // In our case only `POST` requests have a body. Other requests will not set a `Content-Length`.
            // TODO: `Content-Type` must also be set for `POST`
            if (req.method() == POST && !req.has_param("Content-Length"))
            {
                response = HTTP_ERROR(411, host.config()); // Length required
            }
            else if (conn.bytes_read() > req.content_length() || conn.bytes_read() > host.config().max_content_length())
            {
                response = HTTP_ERROR(413, host.config()); // Payload Too Large
            }
            else
            {
                if (!req.has_param("Host"))
                {
                    response = m_servers[conn.sock_fd()].default_host().router().route(req, conn.req_str());
                }
                else
                {
                    // TODO: Check if the port is the same as the listen port I supposed
                    response = host.router().route(req, conn.req_str());
                }
            }

            conn.req_str().clear();

            if (req.is_keep_alive())
                response.add_param("Connection", "keep-alive");

            if (!response.status().is_error())
                ws::log << ws::info << strmethod(req.method()) << " `" << req.path() << "` -> " << NGREEN
                        << response.status().code() << " " << response.status() << RESET << "\n";
            else
                ws::log << ws::info << strmethod(req.method()) << " `" << req.path() << "` -> " << NRED
                        << response.status().code() << " " << response.status() << RESET << "\n";

            response.send(events[i].data.fd, host.config());
            delete response.body();

            conn.set_epollin(m_epollFd);

            // Close the connection if the client either close the connection or don't want to keep
            // it alive.
            if (!req.is_keep_alive() || req.is_closed())
                close_connection(conn);
            else
                keep_alive(conn);
        }
    }

#define TIMEOUT 1000

    while (1)
    {
        size_t n = 0;

        for (std::map<int, Connection>::iterator it = m_connections.begin(); it != m_connections.end(); it++)
        {
            Connection& conn = it->second;
            if (conn.get_last_event() - time() > TIMEOUT)
            {
                close_connection(conn);
                ws::log << ws::dbg << "Connection " << conn.addr() << " timed out\n";
                break;
            }
            n++;
        }

        if (n == m_connections.size())
            break;
    }
}

void Webserv::close_connection(Connection& conn)
{
    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, conn.fd(), NULL) == -1)
        ws::log << ws::err << FILE_INFO << "epoll_ctl(EPOLL_CTL_DEL) failed: " << strerror(errno) << "\n";
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

bool Webserv::has_server(struct sockaddr_in addr)
{
    for (std::map<int, Server>::iterator it = m_servers.begin(); it != m_servers.end(); it++)
    {
        Server& server = it->second;
        struct sockaddr_in serv_addr = server.addr();

        if (serv_addr.sin_port == addr.sin_port && serv_addr.sin_addr.s_addr == addr.sin_addr.s_addr)
            return true;
    }
    return false;
}

Server& Webserv::get_server(struct sockaddr_in addr)
{
    for (std::map<int, Server>::iterator it = m_servers.begin(); it != m_servers.end(); it++)
    {
        Server& server = it->second;
        if (server.addr().sin_port == addr.sin_port && server.addr().sin_addr.s_addr == addr.sin_addr.s_addr)
            return server;
    }
    throw new std::exception();
}
