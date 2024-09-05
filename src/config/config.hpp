#pragma once

#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

#include "config/parser.hpp"
#include "http/request.hpp"

class Location
{
public:
    Location();

    virtual ~Location()
    {
    }

    virtual Result<int, ConfigError> deserialize(ConfigEntry& from);

    std::vector<Method> methods()
    {
        return m_methods;
    }

    std::string& route()
    {
        return m_route;
    }

    std::string& root()
    {
        return m_root;
    }

    bool indexing()
    {
        return m_enable_indexing;
    }

    std::string& default_page()
    {
        return m_default;
    }

    std::map<std::string, std::string>& cgis()
    {
        return m_cgis;
    }

    std::string& upload_dir()
    {
        return m_upload_directory;
    }

private:
    std::string m_route;

    std::vector<Method> m_methods;
    std::string m_root;
    bool m_enable_indexing;
    /* Default page returned if a directory is returned. */
    std::string m_default;
    std::map<std::string, std::string> m_cgis;
    std::string m_upload_directory;
};

class Server
{
public:
    Server();

    virtual ~Server()
    {
    }

    virtual Result<int, ConfigError> deserialize(ConfigEntry& from);

    std::string& server_name()
    {
        return m_server_name;
    }

    struct sockaddr_in& listen_addr()
    {
        return m_listen_addr;
    }

    std::map<int, std::string>& error_pages()
    {
        return m_error_pages;
    }

    size_t max_content_length()
    {
        return m_max_content_length;
    }

private:
    std::string m_server_name;
    struct sockaddr_in m_listen_addr;
    std::map<int, std::string> m_error_pages;

    /* Maximum accepted `Content-Length` */
    size_t m_max_content_length;

    std::vector<Location> m_locations;
};

class Config : public Deser
{
public:
    Config();

    virtual ~Config()
    {
    }

    Result<int, ConfigError> load_from_file(std::string filepath);
    virtual Result<int, ConfigError> deserialize(ConfigEntry& from);

    std::vector<Server> servers()
    {
        return m_servers;
    }

private:
    std::vector<Server> m_servers;
};
