#include "connection.hpp"
#include "webserv.hpp"
#include <netinet/in.h>

Connection::Connection()
{
}

Connection::Connection(int conn, struct sockaddr_in addr, struct epoll_event event)
    : m_addr(addr), m_connection(conn), m_event(event)
{
}

socklen_t& Connection::getAddrLen()
{
    return m_addrLen;
}

int Connection::getConnection() const
{
    return m_connection;
}
