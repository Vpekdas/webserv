#include "request.hpp"
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

std::vector<std::string> _split(const std::string& s, const std::string& delimiter)
{
    std::vector<std::string> parts;
    size_t last = 0;
    size_t next = 0;

    while ((next = s.find(delimiter, last)) != std::string::npos)
    {
        parts.push_back(s.substr(last, next - last));
        last = next + 1;
    }
    parts.push_back(s.substr(last));
    return parts;
}

std::string _trim(std::string s)
{
    size_t start;
    size_t end;

    for (start = 0; start < s.size() && isspace(s[start]); start++)
    {
    }

    for (end = s.size() - 1; end > 0 && isspace(s[end]); end--) // TODO: recode isspace
    {
    }

    return s.substr(start, end);
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
    std::vector<std::string> args_vec = _split(args_str, "&");

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
    std::string header = source.substr(0, source.find(SEP SEP));
    std::vector<std::string> lines = _split(header, SEP);

    // We need at leat the `GET / HTTP/1.1`

    if (lines.size() == 0)
        return Err<Request, int>(0);

    std::vector<std::string> request_line = _split(lines[0], " ");

    if (request_line.size() != 3)
        return Err<Request, int>(0);

    std::string method = request_line[0];
    std::string path = request_line[1];
    std::string protocol = request_line[2]; // HTTP/2.x works differently than HTTP/1.x so this should be checked for

    Request request;

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
    request.m_path = path.substr(0, path.find("="));
    request._parse_path(path);

    for (size_t i = 1; i < lines.size(); i++)
    {
        size_t comma = lines[i].find(":");
        std::string key = _trim(lines[i].substr(0, comma));
        std::string value = _trim(lines[i].substr(comma + 1));

        request.m_params[key] = value;
    }

    return request;
}
