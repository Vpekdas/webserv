#include "request.hpp"
#include "string.hpp"
#include <cctype>
#include <iostream>
#include <vector>

Request::Request()
{
    (void)m_method;
    (void)m_protocol;
    (void)m_body;
}

std::string& Request::get_param(std::string key)
{
    return m_params[key];
}

bool Request::has_param(std::string key)
{
    return m_params.count(key) > 0;
}

/*
    Extract parameters (eg `<url>?foo=bar&test=123`) from the path.
 */
void Request::_parse_path(std::string path)
{
    size_t pos = path.find("?");
    if (pos == std::string::npos)
        return;

    std::string args_str = path.substr(pos);
    std::vector<std::string> args_vec = split(args_str, "&");

    for (size_t i = 0; i < args_vec.size(); i++)
    {
        pos = args_vec[i].find("=");
        std::string key = args_vec[i].substr(0, pos);
        std::string value;

        if (pos == std::string::npos)
            value = "";
        else
            value = args_vec[i].substr(pos + 1);

        m_args[key] = value;
    }
}

Result<Request, int> Request::parse(std::string source)
{
    size_t pos = source.find(SEP SEP);
    std::string header = source.substr(0, pos);
    std::vector<std::string> lines = split(header, SEP);

    std::cout << source << "\n";

    // We need at leat the `GET / HTTP/1.1`

    if (lines.size() == 0)
        return Err<Request, int>(0);

    std::vector<std::string> request_line = split(lines[0], " ");

    if (request_line.size() != 3)
        return Err<Request, int>(0);

    std::string method = request_line[0];
    std::string path = request_line[1];
    std::string protocol = request_line[2]; // HTTP/2.x works differently than HTTP/1.x so this should be checked for

    Request request;
    request.m_header_size = pos + 4; // "\r\n\r\n"

    if (method == "GET")
        request.m_method = GET;
    else if (method == "POST")
        request.m_method = POST;
    else if (method == "DELETE")
        request.m_method = DELETE;
    else if (method == "HEAD")
        request.m_method = HEAD;
    else
        return Err<Request, int>(0);

    request.m_protocol = protocol;
    request.m_path = path.substr(0, path.find("?"));
    request._parse_path(path);

    for (size_t i = 1; i < lines.size(); i++)
    {
        size_t comma = lines[i].find(":");
        std::string key = trim(lines[i].substr(0, comma));
        std::string value = trim(lines[i].substr(comma + 1));

        // std::cout << key << ": " << value << "\n";

        request.m_params[key] = value;
    }

    return request;
}
