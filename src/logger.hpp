#pragma once

#include <cctype>
#include <fstream>
#include <iostream>

#include "colors.hpp"

namespace ws
{
enum LogLevel
{
    INFO = 0,
    WARN = 1,
    ERROR = 2,
    DEBUG = 3
};

class Logger
{
public:
    void init()
    {
        m_file_stream.open("webserv.log");
    }

    void print(std::string value)
    {
        if (m_file_stream.is_open())
        {
            for (size_t i = 0; i < value.size(); i++)
            {
                // Filter shell colors which starts with a '\033' and ends with 'm'
                if (value[i] == '\033')
                {
                    for (; i < value.size() && value[i] != 'm'; i++)
                        ;
                }
                else
                {
                    m_file_stream << value[i];
                }
            }
            m_file_stream.flush();
        }
        std::cerr << value;
    }

    template <typename T>
    void print(T value)
    {
        if (m_file_stream.is_open())
        {
            m_file_stream << value;
            m_file_stream.flush();
        }
        std::cerr << value;
    }

    template <typename T>
    Logger& operator<<(T value)
    {
        print(value);
        return *this;
    }

private:
    std::ofstream m_file_stream;
};

const std::string info = "[" NGREEN "INFO" RESET " ] ";
const std::string warn = "[" NYELLOW "WARN" RESET " ] ";
const std::string err = "[" NRED "ERROR" RESET "] ";
const std::string dbg = "[" NBLUE "DEBUG" RESET "] ";

extern Logger log;
} // namespace ws

#define FILE_INFO YELLOW << __FILE__ << ":" << __LINE__ << RESET << " - "
