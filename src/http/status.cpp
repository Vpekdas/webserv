#include "status.hpp"

HttpStatus::HttpStatus() : m_code(200)
{
}

HttpStatus::HttpStatus(int code) : m_code(code)
{
}

bool HttpStatus::is_error() const
{
    return m_code >= 300;
}

int HttpStatus::code() const
{
    return m_code;
}

std::ostream& operator<<(std::ostream& os, HttpStatus const& error)
{
    switch (error.code())
    {
    case 100:
        os << "Continue";
    case 200:
        os << "OK";
        break;
    case 301:
        os << "Moved Permanently";
        break;
    case 308:
        os << "Permanent Redirect";
        break;
    case 403:
        os << "Forbidden";
        break;
    case 404:
        os << "Not found";
        break;
    case 405:
        os << "Method not allowed";
        break;
    case 411:
        os << "Length required";
        break;
    case 413:
        os << "Payload Too Large";
        break;
    case 500:
        os << "Internal server error";
        break;
    default:
        os << "UNKNOWN"; // TODO: Unreachable
        break;
    }
    return os;
}
