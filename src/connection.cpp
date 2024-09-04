#include "connection.hpp"
#include "webserv.hpp"
#include <netinet/in.h>

Connection::Connection()
{
}

Connection::Connection(int conn, struct sockaddr_in addr, struct epoll_event event)
    : m_addr(addr), m_fd(conn), m_event(event)
{
}

const socklen_t& Connection::getAddrLen() const
{
    return m_addrLen;
}

int Connection::fd() const
{
    return m_fd;
}

std::string& Connection::getReqStr()
{
    return m_reqStr;
}
