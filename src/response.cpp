#include "response.hpp"
#include <fstream>
#include "request.hpp"

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

Response::Response() : m_code(200)
{
}

Response::Response(int code) : m_code(code)
{
    (void) m_body;
}

Response Response::ok(int code, std::string source)
{
    Response response(code);
    response.m_body = source;
    return response;
}

std::string Response::build()
{
    return m_body;
}

static void _replace_all(std::string& src, std::string from, std::string to)
{
    size_t i;

    while ((i = src.find(from)) != std::string::npos)
        src.replace(i, from.size(), to);
}

Response Response::httpcat(int code)
{
    Response response(code);

    std::ifstream ifs("src/error.html");
    if (!ifs.is_open())
        return response;
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    _replace_all(content, "%{error}", SSTR(code));
    response.m_body = content;

    return response;
}
