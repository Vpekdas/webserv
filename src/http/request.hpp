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

    bool is_coffee()
    {
        return m_params.count("User-Agent") > 0 && m_params["User-Agent"].find("coffee") != std::string::npos;
    }

    std::string& get_param(std::string key);

    bool is_keep_alive()
    {
        return m_params.count("Connection") > 0 && m_params["Connection"] == "keep-alive";
    }

    bool is_closed()
    {
        return m_params.count("Connection") > 0 && m_params["Connection"] == "closed";
    }

    std::string& cookies()
    {
        return get_param("Cookie");
    }

    std::string& user_agent()
    {
        return get_param("User-Agent");
    }

    std::string& path()
    {
        return m_path;
    }

    Method method()
    {
        return m_method;
    }

private:
    Method m_method;
    std::string m_path;
    std::map<std::string, std::string> m_params;
    std::string m_protocol;
    std::map<std::string, std::string> m_args;
    std::string m_body;

    void _parse_path(std::string path);
};
