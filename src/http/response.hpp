#pragma once

#include <map>
#include <sstream>
#include <string>

#include "status.hpp"

#define SSTR(x) static_cast<std::ostringstream&>((std::ostringstream() << std::dec << x)).str()

class Response
{
public:
    Response();
    Response(HttpStatus status);

    static Response ok(HttpStatus status, std::string source);
    static Response httpcat(HttpStatus status);

    std::string encode();
    void add_param(std::string key, std::string value);

private:
    HttpStatus m_status;
    std::string m_body;
    std::map<std::string, std::string> m_params;
};
