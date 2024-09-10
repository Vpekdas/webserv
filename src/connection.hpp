#pragma once

#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>

#include "colors.hpp"
#include "http/request.hpp"

class Connection
{
public:
    Connection();

    Connection(int fd, int sock_fd, struct sockaddr_in addr);

    struct sockaddr_in addr();
    int fd() const;

    int sock_fd()
    {
        return m_sock_fd;
    }

    Request& last_request();
    void set_last_request(Request req);

    size_t bytes_read()
    {
        return m_bytes_read;
    }

    void set_bytes_read(size_t s)
    {
        m_bytes_read = s;
    }

    bool waiting_for_body()
    {
        return m_body;
    }

    void set_body(bool b)
    {
        m_body = b;
    }

    std::string& getReqStr();

    void set_epollin(int epoll_fd);
    void set_epollout(int epoll_fd);

private:
    struct sockaddr_in m_addr;
    int m_fd;
    int m_sock_fd;

    std::string m_reqStr;

    /* The entire was read, now we want the body. */
    bool m_body;

    Request m_req;
    size_t m_bytes_read;

    // TODO: Remove this!
    std::string m_body_str;
};
