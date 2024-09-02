#pragma once

#include "file.hpp"
#include "result.hpp"
#include "status.hpp"
#include <map>

class Router
{
public:
    Router(std::string root);

    /*
        Take the path of the request and return the access to a file.
    */
    Result<File, HttpStatus> route(std::string path);

private:
    typedef Result<File, HttpStatus> (*Processor)(std::string path);

    std::string m_root;
    std::map<std::string, Processor> m_processor;
};
