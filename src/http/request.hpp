#pragma once

#include "result.hpp"
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#define SEP "\r\n"

enum Method
{
    GET,
    POST,
    DELETE,
    HEAD
};

inline const char *strmethod(Method method)
{
    switch (method)
    {
    case GET:
        return "GET";
    case HEAD:
        return "HEAD";
    case DELETE:
        return "DELETE";
    case POST:
        return "POST";
    }
}

class Request
{
public:
    static Result<Request, int> parse(std::string body);

    /*
        Parse a request from a multipart/form-data
     */
    static Result<Request, int> parse_part(std::string header);

    Request();
    Request copy_with(std::string path);

    bool is_coffee()
    {
        return m_params.count("User-Agent") > 0 && m_params["User-Agent"].find("coffee") != std::string::npos;
    }

    std::string& get_param(std::string key);
    bool has_param(std::string key);

    bool is_keep_alive()
    {
        return m_params.count("Connection") > 0 && m_params["Connection"] == "keep-alive";
    }

    bool is_closed()
    {
        return m_params.count("Connection") > 0 && m_params["Connection"] == "close";
    }

    size_t content_length()
    {
        return m_params.count("Content-Length") > 0 ? std::atoi(m_params["Content-Length"].c_str()) : (size_t)-1;
    }

    std::string& content_type()
    {
        return m_params["Content-Type"];
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

    size_t header_size()
    {
        return m_header_size;
    }

    void set_args(std::string param);
    std::string args_str();

private:
    Method m_method;
    std::string m_path;
    std::map<std::string, std::string> m_params;
    std::string m_protocol;
    /*
        Request arguments: `?page=1&foo=bar`
     */
    std::map<std::string, std::string> m_args;

    size_t m_header_size;

    void _parse_path(std::string path);
    void _parse_params(std::vector<std::string>& lines, size_t i = 1, bool ignore_invalid = false);
};
