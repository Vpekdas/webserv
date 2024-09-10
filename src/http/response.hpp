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
    static Response http_error(HttpStatus status, const char *func, const char *file, int line);

    /*
        CGIs will starts the response with a few headers value, but without the first line.
     */
    static Response from_cgi(HttpStatus status, std::string str);

    void add_param(std::string key, std::string value);
    void send(int conn);

    HttpStatus status();
    File *body();

    std::string encode_header();

private:
    HttpStatus m_status;
    File *m_body;
    std::map<std::string, std::string> m_params;
};

#ifdef _DEBUG
#define HTTP_ERROR(CODE) Response::http_error(CODE, __FUNCTION__, __FILE__, __LINE__)
#else
#define HTTP_ERROR(CODE) Response::httpcat(CODE)
#endif
