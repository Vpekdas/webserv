#include <netinet/in.h>
#include <string.h>

#include "connection.hpp"
#include "logger.hpp"

Connection::Connection()
{
}

Connection::Connection(int conn, int sock_fd, struct sockaddr_in addr)
    : m_addr(addr), m_fd(conn), m_sock_fd(sock_fd), m_last_event(0)
{
}

int Connection::fd() const
{
    return m_fd;
}

void Connection::set_epollin(int epoll_fd)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    event.data.fd = m_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, m_fd, &event) == -1)
    {
        ws::log << ws::err << "epoll_ctrl() failed: " << strerror(errno) << "\n";
        // TODO: We should close the connection if epoll_ctrl can't do its stuff.
    }
}

void Connection::set_epollout(int epoll_fd)
{
    struct epoll_event event;
    event.events = EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    event.data.fd = m_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, m_fd, &event) == -1)
    {
        ws::log << ws::err << "epoll_ctrl() failed: " << strerror(errno) << "\n";
        // TODO: We should close the connection if epoll_ctrl can't do its stuff.
    }
}
