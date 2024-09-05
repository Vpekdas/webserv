#pragma once

#include "config/config.hpp"
#include "connection.hpp"
#include "router.hpp"

#include <csignal>
#include <cstdio>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENTS 5
#define READ_SIZE 512

enum Status
{
    SUCCESS = 0,
    FAILURE = -1
};

class Webserv
{
public:
    Webserv();

    int getEpollFd() const;
    int getSockFd() const;

    Result<Connection, int> acceptConnection();

    int initialize(std::string config_path);
    void eventLoop();

    void setEventCount(int eventCount);

    void close_connection(Connection& conn);
    void keep_alive(Connection& conn);

private:
    int m_epollFd;
    int m_sockFd;
    std::map<int, Connection> m_connections;
    struct sockaddr_in m_sockAddr;

    Config m_config;
    Router m_router;
};
