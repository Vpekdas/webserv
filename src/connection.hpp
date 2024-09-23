#pragma once

#include <netinet/in.h>
#include <sys/epoll.h>

#include "http/request.hpp"
#include "option.hpp"

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

    void setReq(Request req)
    {
        m_req = req;
    }

    Option<Request>& req()
    {
        return m_req;
    }

    void clearReq()
    {
        m_req = Option<Request>();
    }

    bool set_epollin(int epoll_fd);
    bool set_epollout(int epoll_fd);

private:
    struct sockaddr_in m_addr;
    int m_fd;
    int m_sock_fd;

    std::string m_reqStr;

    int64_t m_last_event;
    Option<Request> m_req;
};
