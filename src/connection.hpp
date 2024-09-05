#pragma once

#include "colors.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>

class Connection
{
public:
    Connection();

    Connection(int fd, struct sockaddr_in addr, struct epoll_event event);

    const sockaddr_in& getSockAddr() const;
    const socklen_t& getAddrLen() const;
    int fd() const;

    std::string& getReqStr();

private:
    struct sockaddr_in m_addr;
    socklen_t m_addrLen;
    int m_fd;
    struct epoll_event m_event;

    std::string m_reqStr;
};
