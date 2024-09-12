#pragma once

#include <iostream>
#include <map>

#include "config/config.hpp"
#include "router.hpp"

class Host
{
public:
    Host()
    {
    }

    Host(ServerConfig config) : m_config(config), m_router(config)
    {
    }

    ServerConfig& config()
    {
        return m_config;
    }

    Router& router()
    {
        return m_router;
    }

private:
    ServerConfig m_config;
    Router m_router;
};

class Server
{
public:
    Server();
    Server(struct sockaddr_in addr);
    ~Server();

    int sock_fd();

    struct sockaddr_in addr()
    {
        return m_addr;
    }

    bool has_host(std::string& host)
    {
        return m_hosts.count(host) > 0;
    }

    Host& default_host()
    {
        return m_hosts.begin()->second;
    }

    Host& host(std::string host)
    {
        return m_hosts[host];
    }

    void add_host(std::string host, ServerConfig config)
    {
        m_hosts[host] = Host(config);
    }

public:
    int m_sock_fd;
    struct sockaddr_in m_addr;
    std::map<std::string, Host> m_hosts;
};
