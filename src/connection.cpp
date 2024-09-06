#include "connection.hpp"
#include "webserv.hpp"
#include <netinet/in.h>

Connection::Connection()
{
}

Connection::Connection(int conn, struct sockaddr_in addr) : m_addr(addr), m_fd(conn), m_body(false), m_bytes_read(0)
{
}

struct sockaddr_in Connection::addr()
{
    return m_addr;
}

int Connection::fd() const
{
    return m_fd;
}

std::string& Connection::getReqStr()
{
    return m_reqStr;
}

Request& Connection::last_request()
{
    return m_req;
}

void Connection::set_last_request(Request req)
{
    m_req = req;
}
