#pragma once

#include <map>

#include "cgi/cgi.hpp"
#include "config/config.hpp"
#include "http/request.hpp"
#include "http/response.hpp"

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
    Response route(Request& req);

private:
    std::map<std::string, CGI> m_cgis;
    ServerConfig m_config;

    Response _route_with_location(Request& req, Location& loc);
    Response _directory_listing(Request& req, Location& loc, std::string path);
};
