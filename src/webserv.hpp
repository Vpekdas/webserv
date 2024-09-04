#pragma once

#include "../src/colors.hpp"
#include <iostream>
#include <vector>

#include "connection.hpp"
#include "file.hpp"
#include "router.hpp"
#include "smart_pointers.hpp"

#include "http/request.hpp"
#include "http/response.hpp"

#include <csignal>
#include <cstdio>
#include <errno.h>
#include <map>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
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

    int initialize();
    void eventLoop();

    void setEventCount(int eventCount);

    void close_connection(Connection& conn);
    void keep_alive(Connection& conn);

private:
    int m_epollFd;
    int m_sockFd;
    std::map<int, Connection> m_connections;
    struct sockaddr_in m_sockAddr;

    Router m_router;
};
