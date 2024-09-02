#pragma once

#include <ostream>

class HttpStatus
{
public:
    HttpStatus();
    HttpStatus(int code);

    bool is_error() const;
    int code() const;

private:
    int m_code;
};

std::ostream& operator<<(std::ostream& os, HttpStatus const& error);
