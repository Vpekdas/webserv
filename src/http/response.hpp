#pragma once

#include <map>
#include <string>

#include "file.hpp"
#include "status.hpp"

class Response
{
public:
    Response();
    Response(HttpStatus status);

    static Response ok(HttpStatus status, File *file);
    static Response httpcat(HttpStatus status);

    /*
        CGIs will starts the response with a few headers value, but without the first line.
     */
    static Response from_cgi(HttpStatus status, std::string str);

    void add_param(std::string key, std::string value);
    void send(int conn);

    HttpStatus status();

    std::string encode_header();

private:
    HttpStatus m_status;
    File *m_body;
    std::map<std::string, std::string> m_params;
};
