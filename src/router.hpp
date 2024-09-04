#pragma once

#include "cgi/cgi.hpp"
#include "file.hpp"
#include "result.hpp"
#include "smart_pointers.hpp"
#include "http/status.hpp"
#include <map>

class Router
{
public:
    Router(std::string root);

    /*
        Take the path of the request and return the access to a file.
    */
    Result<File *, HttpStatus> route(std::string path);

private:
    std::string m_root;
    std::map<std::string, CGI> m_cgis;
};
