#include <cstddef>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "file.hpp"
#include "logger.hpp"
#include "request.hpp"
#include "response.hpp"
#include "string.hpp"

// clang-format off
#ifndef _DEBUG
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
"        <h2>Here's a %{error_theme} to make you feel better</h3>" SEP
"        <img src='%{error_url}/%{error}.jpg' style='height: 70%'>" SEP
"    </body>" SEP
"</html>" SEP;
#else
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
"        <h2>Here's a %{error_theme} to make you feel better</h3>" SEP
"        <h2 style='color: yellow;'>in %{func}() at <a style='color: yellow;' href='vscode://file/%{path}/%{file}:%{line}'>%{file}:%{line}</a></h2>" SEP
"        <img src='%{error_url}/%{error}.jpg' style='width: 45%;'>" SEP
"    </body>" SEP
"</html>" SEP;
#endif
// clang-format on

std::map<std::string, std::string> Response::m_themes;

void Response::_build_themes()
{
    m_themes["cat"] = "https://http.cat";
    m_themes["dog"] = "https://http.dog";
    m_themes["duck"] = "https://httpducks.com";
    m_themes["goat"] = "https://httpgoats.com";
    m_themes["garden"] = "https://http.garden";
    m_themes["pizza"] = "https://http.pizza";
    m_themes["fish"] = "https://http.fish";
}

Response::Response() : m_status(200)
{
}

Response::Response(HttpStatus status) : m_status(status)
{
    (void)m_body;
}

Response Response::ok(HttpStatus status, File file)
{
    Response response(status);
    response.m_body = file;

    response.add_param("Content-Length", to_string(file.file_size()));
    response.add_param("Content-Type", file.mime());

    return response;
}

Response Response::from_cgi(HttpStatus status, std::string str)
{
    Response response;
    response.m_status = status;

    size_t pos = str.find(SEP SEP);
    std::string header = str.substr(0, pos);
    std::string body = str.substr(pos + 2);
    std::vector<std::string> lines = split(header, SEP);

    // if (lines.size() == 0)
    //     return Err<Request, int>(0);

    std::vector<std::string> request_line = split(lines[0], " ");

    for (size_t i = 0; i < lines.size(); i++)
    {
        size_t comma = lines[i].find(":");
        std::string key = lines[i].substr(0, comma);
        std::string value = trim(lines[i].substr(comma + 1));

        response.m_params[key] = value;
    }

    response.m_body = File::memory(body, "text/html");
    response.add_param("Content-Length", to_string(response.body().file_size()));

    return response;
}

HttpStatus Response::status()
{
    return m_status;
}

File& Response::body()
{
    return m_body;
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

bool Response::send(int conn, ServerConfig& config)
{
    if (!m_body.exists())
    {
        ws::log << ws::err << FILE_INFO << "Attempted to send a invalid response\n";
        Response err = HTTP_ERROR(500, config); // Internal server error
        err.send(conn, config);
        return false;
    }

    std::string header = encode_header();

    ssize_t s = ::send(conn, header.c_str(), header.size(), 0);

    if (s == -1 || s == 0)
    {
        return false;
    }

    if (!m_body.send(conn))
    {
        return false;
    }

    return true;
}

void Response::add_param(std::string key, std::string value)
{
    m_params[key] = value;
}

Response Response::http_error(HttpStatus status, ServerConfig& config, const char *func, const char *file, int line)
{
    (void)func;
    (void)file;
    (void)line;

    Response response(status);

    if (config.error_pages().count(status.code()) == 0 ||
        access(config.error_pages()[status.code()].c_str(), F_OK | R_OK) == -1)
    {
        std::string content = source;

        std::string theme;
        if (m_themes.count(config.error_theme()) > 0)
            theme = config.error_theme();
        else
            theme = "cat";

        replace_all(content, "%{error_url}", m_themes[theme]);
        replace_all(content, "%{error_theme}", theme);
        replace_all(content, "%{error}", to_string(status.code()));

#ifdef _DEBUG
        replace_all(content, "%{func}", func);
        replace_all(content, "%{file}", file);
        replace_all(content, "%{line}", to_string(line));

        char buf[64];

        getcwd(buf, 64);
        replace_all(content, "%{path}", buf);
#endif

        response.m_body = File::memory(content, "text/html");

        response.add_param("Content-Type", "text/html");
        response.add_param("Content-Length", to_string(response.body().file_size()));
    }
    else
    {
        response.m_body = File::stream(config.error_pages()[status.code()]);

        response.add_param("Content-Type", "text/html");
        response.add_param("Content-Length", to_string(response.body().file_size()));
    }

    if (response.status().code() >= 400)
        response.add_param("Connection", "close");

    return response;
}
