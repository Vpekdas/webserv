#pragma once

#include <ios>
#include <sstream>
#include <string>

template <typename T>
std::string to_string(T value)
{
    return (std::stringstream() << value).str();
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
