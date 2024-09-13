#pragma once

#include <map>
#include <string>

#include "config/config.hpp"
#include "file.hpp"
#include "status.hpp"

class Response
{
public:
    Response();

    static Response ok(HttpStatus status, File *file);
    static Response http_error(HttpStatus status, ServerConfig& config, const char *func, const char *file, int line);

    /*
        CGIs will starts the response with a few headers value, but without the first line.
     */
    static Response from_cgi(HttpStatus status, std::string str);

    void add_param(std::string key, std::string value);
    void send(int conn, ServerConfig& config);

    HttpStatus status();
    File *body();

    std::string encode_header();

private:
    HttpStatus m_status;
    File *m_body;
    std::map<std::string, std::string> m_params;

    Response(HttpStatus status);
};

#define HTTP_ERROR(CODE, CONF) Response::http_error(CODE, CONF, __FUNCTION__, __FILE__, __LINE__)
