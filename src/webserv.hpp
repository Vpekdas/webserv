#pragma once

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <map>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/config.hpp"
#include "connection.hpp"
#include "server.hpp"

#define MAX_EVENTS 128
#define READ_SIZE 512

class Webserv
{
public:
    Webserv();

    int getEpollFd() const;

    Result<Connection, int> acceptConnection(int sock_fd);

    int initialize(std::string config_path);
    void eventLoop();

    void quit();

    void close_connection(Connection& conn);
    void keep_alive(Connection& conn);

private:
    int m_epollFd;
    std::map<int, Connection> m_connections;

    bool m_running;

    Config m_config;
    std::map<int, Server> m_servers;

    void poll_events();

    bool has_server(struct sockaddr_in addr);
    Server& get_server(struct sockaddr_in addr);
};
