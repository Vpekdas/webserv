#pragma once

#include "config/config.hpp"
#include <iostream>

class Server
{
public:
    Server(ServerConfig& config);
    ~Server();

    int sock_fd();
    ServerConfig& config();

private:
    ServerConfig& m_config;
    int m_sock_fd;
};
