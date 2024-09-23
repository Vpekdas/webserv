#include "request.hpp"
#include "string.hpp"
#include <cctype>
#include <iostream>
#include <vector>

Request::Request()
{
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
    std::string args_str = path;
    std::vector<std::string> args_vec = split(args_str, "&");

    for (size_t i = 0; i < args_vec.size(); i++)
    {
        size_t pos = args_vec[i].find("=");
        std::string key = args_vec[i].substr(0, pos);
        std::string value;

        if (pos == std::string::npos)
            value = "";
        else
            value = args_vec[i].substr(pos + 1);

        m_args[key] = value;
    }
}

void Request::_parse_params(std::vector<std::string>& lines, size_t i, bool ignore_invalid)
{
    (void)ignore_invalid;
    for (; i < lines.size(); i++)
    {
        if (lines[i].empty())
            continue;
        size_t comma = lines[i].find(":");
        std::string key = trim(lines[i].substr(0, comma));
        std::string value = trim(lines[i].substr(comma + 1));

        m_params[key] = value;
    }
}

Result<Request, int> Request::parse(std::string source)
{
    size_t pos = source.find(SEP SEP);
    std::string header = source.substr(0, pos);
    std::string body = source.substr(pos + 4);
    std::vector<std::string> lines = split(header, SEP);

    // We need at least the `GET / HTTP/1.1`

    if (lines.size() == 0)
        return Err<Request, int>(0);

    std::vector<std::string> request_line = split(lines[0], " ");

    if (request_line.size() != 3)
        return Err<Request, int>(0);

    std::string method = request_line[0];
    std::string path = request_line[1];
    std::string protocol = request_line[2];

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
    request.m_body = body;
    request.m_path = path.substr(0, path.find("?"));
    request._parse_path(path.substr(1));

    request._parse_params(lines);

    return request;
}

Result<Request, int> Request::parse_part(std::string header)
{
    std::vector<std::string> lines = split(header, "\r\n");

    Request req;
    req._parse_params(lines, 0, true);
    return req;
}

void Request::set_args(std::string param)
{
    _parse_path(param);
}

std::string Request::args_str()
{
    std::string s = "";

    for (std::map<std::string, std::string>::iterator it = m_args.begin(); it != m_args.end(); it++)
    {
        s += it->first + "=" + it->second;
        std::map<std::string, std::string>::iterator it2 = it;
        it2++;
        if (it2 != m_args.end())
            s += "&";
    }

    return s;
}
