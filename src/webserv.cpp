#include "webserv.hpp"
#include "config/config.hpp"
#include "connection.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "logger.hpp"
#include "server.hpp"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
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

        if (config.server_name().is_none() || config.server_name().unwrap().empty())
        {
            ws::log << ws::err << "Missing `server_name` in config\n";
            continue;
        }

        if (config.listen_addr().is_none())
        {
            ws::log << ws::err << "Missing `listen_addr` in config\n";
            continue;
        }

        std::string host = config.server_name().unwrap();

        if (has_server(config.listen_addr().unwrap()))
        {
            Server& server = get_server(config.listen_addr().unwrap());
            std::string host = config.server_name().unwrap();

            if (server.has_host(host))
            {
                ws::log << ws::err << "Duplicated server hostname " << host << "\n";
                continue;
            }

            server.add_host(host, config);
        }
        else
        {
            Server server(config.listen_addr().unwrap());
            server.add_host(host, config);

            if (server.sock_fd() == -1)
                continue;

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

        struct sockaddr_in addr = config.listen_addr().unwrap();
        ws::log << ws::info << "Host " BIWHITE << host << RESET " listening on " << addr << "\n";
    }

    if (m_servers.empty())
    {
        ws::log << ws::err << "No servers running, stopping now...\n";
        close(m_epollFd);
        return;
    }

    while (m_running)
    {
        poll_events();
    }

    closeFds();
}

void Webserv::closeFds()
{
    // Close all servers currently listening.
    for (std::map<int, Server>::iterator it = m_servers.begin(); it != m_servers.end(); it++)
        close(it->second.sock_fd());

    // Close all remaining connections.
    for (std::map<int, Connection>::iterator it = m_connections.begin(); it != m_connections.end(); it++)
        close(it->second.fd());

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
            Result<Connection, int> res = acceptConnection(events[i].data.fd);
            if (res.is_err())
                continue;

            Connection conn = res.unwrap();
            conn.set_last_event(time());
            m_connections[conn.fd()] = conn;
            continue;
        }

        if ((events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)))
        {
            Connection& conn = m_connections[events[i].data.fd];
            closeConnection(conn);
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
                closeConnection(conn);
                continue;
            }

            req_str.append(buf, n);

            if (conn.req().is_some() || req_str.find(SEP SEP) != std::string::npos)
            {
                Request req;

                if (conn.req().is_some())
                {
                    req = conn.req().unwrap();
                }
                else
                {
                    std::string header = req_str.substr(0, req_str.find(SEP SEP) + 4);
                    Result<Request, int> res = Request::parse(header);

                    // The client send us a invalid HTTP request.
                    if (res.is_err())
                    {
                        closeConnection(conn);
                        continue;
                    }

                    conn.setReq(res.unwrap());
                    req = conn.req().unwrap();
                }

                size_t contentLength = req.content_length();

                if (req.method() != POST || conn.req_str().size() - req.header_size() >= contentLength)
                    conn.set_epollout(m_epollFd);
            }
        }

        else if ((events[i].events & EPOLLOUT))
        {
            Connection& conn = m_connections[events[i].data.fd];
            Result<Request, int> res = Request::parse(conn.req_str());

            if (res.is_err())
            {
                ws::log << ws::dbg << "Invalid HTTP request\n";
                ws::log << conn.req_str() << "\n";
                closeConnection(conn);
                continue;
            }

            Request req = res.unwrap();

            std::string hostString = req.get_param("Host").substr(0, req.get_param("Host").find(':'));
            Host& host = m_servers[conn.sock_fd()].default_host();

            if (m_servers[conn.sock_fd()].has_host(hostString))
                host = m_servers[conn.sock_fd()].host(hostString);

            Response response;
            // In our case only `POST` requests have a body. Other requests will not set a `Content-Length`.
            // TODO: `Content-Type` must also be set for `POST`
            if (req.method() == POST && !req.has_param("Content-Length"))
            {
                response = HTTP_ERROR(411, host.config()); // Length required
            }
            else if (/*conn.req_str().size() > req.content_length() ||*/
                     conn.req_str().size() > host.config().max_content_length())
            {
                response = HTTP_ERROR(413, host.config()); // Payload Too Large
            }
            else
            {
                if (!req.has_param("Host"))
                {
                    response = m_servers[conn.sock_fd()].default_host().router().route(req);
                }
                else
                {
                    // TODO: Check if the port is the same as the listen port I supposed
                    response = host.router().route(req);
                }
            }

            conn.req_str().clear();
            conn.clearReq();

            if (req.is_keep_alive())
                response.add_param("Connection", "keep-alive");

            if (!response.status().is_error())
                ws::log << ws::info << strmethod(req.method()) << " `" << req.path() << "` -> " << NGREEN
                        << response.status().code() << " " << response.status() << RESET << "\n";
            else
                ws::log << ws::info << strmethod(req.method()) << " `" << req.path() << "` -> " << NRED
                        << response.status().code() << " " << response.status() << RESET << "\n";

            // Close the connection if the client close the connection, we don't want to keep
            // it alive or there was an error while sending the response.
            if (!response.send(events[i].data.fd, host.config()) || !req.is_keep_alive() || req.is_closed() ||
                response.get_param("Connection") == "close")
                closeConnection(conn);
            else
                conn.set_epollin(m_epollFd);
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
                closeConnection(conn);
                ws::log << ws::dbg << "Connection " << conn.addr() << " timed out\n";
                break;
            }
            n++;
        }

        if (n == m_connections.size())
            break;
    }
}

void Webserv::closeConnection(Connection& conn)
{
    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, conn.fd(), NULL) == -1)
        ws::log << ws::err << FILE_INFO << "epoll_ctl(EPOLL_CTL_DEL) failed: " << strerror(errno) << "\n";
    close(conn.fd());
    m_connections.erase(m_connections.find(conn.fd()));
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
