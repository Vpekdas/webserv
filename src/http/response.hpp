#pragma once

#include <map>
#include <string>
#include <sstream>

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

class Response
{
public:
    Response();
    Response(int code);

    static Response ok(int code, std::string source);
    static Response httpcat(int code);

    std::string encode();
    void add_param(std::string key, std::string value);

private:
    int m_code;
    std::string m_body;
    std::map<std::string, std::string> m_params;
};
