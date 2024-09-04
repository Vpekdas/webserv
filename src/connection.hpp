#pragma once

#include "webserv.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>

class Connection
{
public:
    Connection();

    Connection(int conn, struct sockaddr_in addr, struct epoll_event event);

    const sockaddr_in& getSockAddr() const;
    socklen_t& getAddrLen();
    int getConnection() const;

private:
    struct sockaddr_in m_addr;
    socklen_t m_addrLen;
    int m_connection;
    struct epoll_event m_event;
};
