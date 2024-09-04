#include "response.hpp"
#include "file.hpp"
#include "request.hpp"
#include "smart_pointers.hpp"
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>

// clang-format off
static std::string source =
"<!DOCTYPE html>" SEP
"<html lang='en'>" SEP
"    <head>" SEP
"        <meta charset='UTF-8'>" SEP
"        <meta name='viewport' content='width=device-width, initial-scale=1.0'>" SEP
"        <title>Error %{error}</title>" SEP
"        <style>" SEP
"            * {" SEP
"                background-color: black;" SEP
"            }" SEP
"" SEP
"            h1, h2, h3 {" SEP
"                color: white;" SEP
"                font-family: Verdana, Geneva, Tahoma, sans-serif;" SEP
"            }" SEP
"" SEP
"            body {" SEP
"                text-align: center;" SEP
"            }" SEP
"        </style>" SEP
"    </head>" SEP
"    <body>" SEP
"        <h1>Oops, there seems to be an error !</h1>" SEP
"        <h2>Here's a cat to make you feel better</h3>" SEP
"        <img src='https://http.cat/%{error}'>" SEP
"    </body>" SEP
"</html>" SEP;
// clang-format on

Response::Response() : m_status(200)
{
}

Response::Response(HttpStatus status) : m_status(status)
{
    (void)m_body;
}

Response Response::ok(HttpStatus status, SharedPtr<File> file)
{
    Response response(status);
    response.m_body = file;

    response.add_param("Content-Length", SSTR(file->file_size()));

    std::cout << NRED << "name = `" << file->file_name() << "`, mime = " << file->mime() << "\n";

    response.add_param("Content-Type", file->mime());

    return response;
}

std::string Response::encode_header()
{
    std::stringstream r;

    r << "HTTP/1.1 " << m_status.code() << " " << m_status << SEP;

    for (std::map<std::string, std::string>::iterator it = m_params.begin(); it != m_params.end(); it++)
        r << it->first << ": " << it->second << SEP;

    r << SEP;

    return r.str();
}

void Response::send(int conn)
{
    std::string header = encode_header();

    // TODO: Check errors (including SIGPIPE)
    ssize_t s = ::send(conn, header.c_str(), header.size(), 0);
    (void)s;

    m_body->send(conn);
}

void Response::add_param(std::string key, std::string value)
{
    m_params[key] = value;
}

static void _replace_all(std::string& src, std::string from, std::string to)
{
    size_t i;

    while ((i = src.find(from)) != std::string::npos)
        src.replace(i, from.size(), to);
}

Response Response::httpcat(HttpStatus status)
{
    Response response(status);
    std::string content = source;

    _replace_all(content, "%{error}", SSTR(status.code()));
    response.m_body = make_shared<File>(new StringFile(content, "text/html"));

    response.add_param("Content-Type", "text/html");
    response.add_param("Content-Length", SSTR(response.m_body->file_size()));

    return response;
}
