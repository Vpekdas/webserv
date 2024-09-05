#pragma once

#include "cgi/cgi.hpp"
#include "config/config.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/status.hpp"
#include "result.hpp"
#include <map>

class Router
{
public:
    Router()
    {
    }

    Router(ServerConfig config);

    /*
        Take the path of the request and return the access to a file.
    */
    Result<Response, HttpStatus> route(Request& req);

private:
    std::map<std::string, CGI> m_cgis;
    ServerConfig m_config;

    Result<Response, HttpStatus> _route_with_location(Request& req, Location& loc);
};
