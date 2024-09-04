#pragma once

#define MAX_EVENTS 5
#define READ_SIZE 1024

#include "../src/colors.hpp"
#include <iostream>
#include <vector>

#include "file.hpp"
#include "router.hpp"
#include "smart_pointers.hpp"

#include "http/request.hpp"
#include "http/response.hpp"
#include <csignal>

#include <cstdio>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

enum Status
{
    SUCCESS = 0,
    FAILURE = -1
};

class Webserv
{
public:
    Webserv();
    Webserv(const Webserv& other);
    Webserv& operator=(const Webserv& other);
    ~Webserv();

    int getEpollFd() const;
    int getSockFd() const;

    int initialize();

protected:
private:
    int m_epollFd;
    int m_sockFd;
    std::vector<sockaddr_in> m_sockaddr;
    struct epoll_event events[MAX_EVENTS];
};
