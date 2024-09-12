#pragma once

#include <netinet/in.h>
#include <sys/epoll.h>

#include "http/request.hpp"

class Connection
{
public:
    Connection();

    Connection(int fd, int sock_fd, struct sockaddr_in addr);

    struct sockaddr_in& addr()
    {
        return m_addr;
    }

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

    std::string& req_str()
    {
        return m_reqStr;
    }

    void set_req_str(std::string s)
    {
        m_reqStr = s;
    }

    void set_last_event(int64_t i)
    {
        m_last_event = i;
    }

    int64_t get_last_event()
    {
        return m_last_event;
    }

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

    int64_t m_last_event;
};
