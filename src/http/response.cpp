#include "response.hpp"
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

struct A
{
    int code;
    std::string message;
};

static A messages[] = {
    {   0, "" },
    { 200, "OK" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 418, "I'm a teapot" },
};

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

static std::string& code_to_str(int code)
{
    for (size_t i = 0; i < sizeof(messages) / sizeof(A); i++)
    {
        if (messages[i].code == code)
            return messages[i].message;
    }
    return messages[0].message;
}

std::string Response::encode()
{
    std::string r;

    r += "HTTP/1.1 " + SSTR(m_code) + " " + code_to_str(m_code) + SEP;
    r += SEP;
    r += m_body;

    return r;
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
    std::string content = source;

    _replace_all(content, "%{error}", SSTR(code));
    response.m_body = content;

    return response;
}
