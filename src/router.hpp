#pragma once

#include "cgi/cgi.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/status.hpp"
#include "result.hpp"
#include <map>

class Router
{
public:
    Router(std::string root);

    /*
        Take the path of the request and return the access to a file.
    */
    Result<Response, HttpStatus> route(Request& req);

private:
    std::string m_root;
    std::map<std::string, CGI> m_cgis;
};
