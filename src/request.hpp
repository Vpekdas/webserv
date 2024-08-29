#pragma once

#include "result.hpp"
#include <map>
#include <string>

#define SEP "\r\n"

enum Method
{
    GET,
    POST
};

class Request
{
public:
    static Result<Request, int> parse(std::string body);

    Request();

// private:
    Method m_method;
    std::string m_path;
    std::string m_protocol;
    std::map<std::string, std::string> m_values;
    std::string m_body;
};
