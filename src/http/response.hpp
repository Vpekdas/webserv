#pragma once

#include <map>
#include <sstream>
#include <string>

#include "file.hpp"
#include "smart_pointers.hpp"
#include "status.hpp"

#define SSTR(x) static_cast<std::ostringstream&>((std::ostringstream() << std::dec << x)).str()

class Response
{
public:
    Response();
    Response(HttpStatus status);

    static Response ok(HttpStatus status, SharedPtr<File> file);
    static Response httpcat(HttpStatus status);

    void add_param(std::string key, std::string value);
    void send(int conn);

    std::string encode_header();

private:
    HttpStatus m_status;
    SharedPtr<File> m_body;
    std::map<std::string, std::string> m_params;
};
