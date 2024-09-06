#pragma once

#include "config/config.hpp"
#include "router.hpp"
#include <iostream>

class Server
{
public:
    Server();
    Server(ServerConfig config);
    ~Server();

    int sock_fd();
    ServerConfig& config();
    Router& router();

private:
    ServerConfig m_config;
    int m_sock_fd;
    Router m_router;
};
