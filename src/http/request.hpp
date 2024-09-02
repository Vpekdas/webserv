#pragma once

#include "result.hpp"
#include <map>
#include <string>

#define SEP "\r\n"

enum Method
{
    GET,
    POST,
    DELETE,
    HEAD
};

class Request
{
public:
    static Result<Request, int> parse(std::string body);

    Request();
    bool is_coffee() { return m_params.count("User-Agent") > 0 && m_params["User-Agent"].find("coffee") != std::string::npos; }
    std::string& get_param(std::string key);

public:
    Method m_method;
    std::string m_path;
    std::map<std::string, std::string> m_params;
    std::string m_protocol;
    std::map<std::string, std::string> m_args;
    std::string m_body;

    void _parse_path(std::string path);
};
