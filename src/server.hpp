#pragma once

#include "config/config.hpp"
#include <iostream>

class Server
{
public:
    Server(ServerConfig& config);
    ~Server();

private:
    ServerConfig& m_config;
    int m_sock_fd;
};
