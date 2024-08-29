#include "request.hpp"
#include <iostream>
#include <vector>

Request::Request()
{
    (void) m_method;
    (void) m_protocol;
    (void) m_body;
}

std::vector<std::string> _split(const std::string& s, const std::string& delimiter)
{
    std::vector<std::string> parts;
    size_t last = 0;
    size_t next = 0;
    
    while ((next = s.find(delimiter, last)) != std::string::npos)
    {
        parts.push_back(s.substr(last, next-last));
        last = next + 1;
    }
    parts.push_back(s.substr(last));
    return parts;
}

std::string _trim(std::string s)
{
    size_t start;
    size_t end;

    for (start = 0; start < s.size() && s[start] == ' '; start++)
    {
    }

    for (end = s.size() - 1; end > 0 && s[end] == ' '; end--)
    {
    }

    return s.substr(start, end + 1);
}

Result<Request, int> Request::parse(std::string source)
{
    std::string header = source.substr(0, source.find(SEP SEP));
    std::vector<std::string> lines = _split(header, SEP);

    // We need at leat the `GET / HTTP/1.1`

    if (lines.size() == 0) return Err<Request, int>(0);

    std::vector<std::string> first_line = _split(lines[0], " ");

    if (first_line.size() != 3) return Err<Request, int>(0);

    std::string method = first_line[0];
    std::string path = first_line[1];
    std::string protocol = first_line[2]; // HTTP/2.x works differently than HTTP/1.x so this should be checked for

    Request request;

    if (method == "GET") request.m_method = GET;
    else if (method == "POST") request.m_method = POST;
    else return Err<Request, int>(0);

    request.m_path = path;
    request.m_protocol = protocol;

    for (size_t i = 1; i < lines.size(); i++)
    {
        size_t comma = lines[i].find(":");
        std::string key = lines[i].substr(0, comma);
        std::string value = _trim(lines[i].substr(comma + 1));

        request.m_values[key] = value;
    }

    return request;
}
