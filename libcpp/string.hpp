#pragma once

#include <ios>
#include <sstream>
#include <string>
#include <vector>

template <typename T>
std::string to_string(T value)
{
    std::stringstream s;
    s << value;
    return s.str();
}

template <typename T>
std::string to_string(T value, const int base)
{
    std::stringstream s;

    if (base == 10)
        s << std::dec;
    else if (base == 16)
        s << std::hex;
    else if (base == 8)
        s << std::oct;
    s << value;

    return s.str();
}

inline std::vector<std::string> split(std::string s, std::string delimiter)
{
    std::vector<std::string> parts;
    size_t last = 0;
    size_t next = 0;

    while ((next = s.find(delimiter, last)) != std::string::npos)
    {
        parts.push_back(s.substr(last, next - last));
        last = next + delimiter.size();
    }
    parts.push_back(s.substr(last));
    return parts;
}

inline std::vector<std::string> split(std::string s, char delimiter)
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

inline std::string trim(std::string s)
{
    size_t start;
    size_t end;

    for (start = 0; start < s.size() && std::isspace(s[start]); start++)
    {
    }

    for (end = s.size() - 1; end > 0 && std::isspace(s[end]); end--)
    {
    }
    end++;

    return s.substr(start, end);
}

inline void replace_all(std::string& src, std::string from, std::string to)
{
    size_t i;

    while ((i = src.find(from)) != std::string::npos)
        src.replace(i, from.size(), to);
}
