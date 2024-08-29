#include "request.hpp"
#include "response.hpp"
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

const std::string source =
    "GET / HTTP/1.1\n"
    "Content-Type: application/json\n"
    "\n\n"
    "{\n"
    "    \"foo\": \"bar\"\n"
    "}\n"
;

int main()
{
    std::cout << "\n";

    // Request request = Request::parse(source).unwrap();

    // std::cout << "method = " << request.m_method << "\n";
    // std::cout << "protocol = " << request.m_protocol << "\n";
    // std::cout << "\n";
    // std::cout << "values:\n";
    // for (std::map<std::string, std::string>::iterator it = request.m_values.begin(); it != request.m_values.end(); it++)
    //     std::cout << "key = " << it->first << ", value = " << it->second << "\n";

    std::cout << Response::httpcat(418).encode() << "\n";
}
