#pragma once

#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

#include "config/parser.hpp"
#include "http/request.hpp"
#include "option.hpp"

class Location
{
public:
    Location();

    virtual ~Location()
    {
    }

    virtual Result<int, ConfigError> deserialize(ConfigEntry& from);

    std::vector<Method>& methods()
    {
        return m_methods;
    }

    std::string& route()
    {
        return m_route;
    }

    Option<std::string>& root()
    {
        return m_root;
    }

    bool indexing()
    {
        return m_enable_indexing;
    }

    Option<std::string>& default_page()
    {
        return m_default;
    }

    std::map<std::string, std::string>& cgis()
    {
        return m_cgis;
    }

    Option<std::string>& upload_dir()
    {
        return m_upload_directory;
    }

    Option<std::string>& redirect()
    {
        return m_redirect;
    }

private:
    std::string m_route;

    std::vector<Method> m_methods;
    Option<std::string> m_root;
    bool m_enable_indexing;
    /* Default page returned if a directory is returned. */
    Option<std::string> m_default;
    std::map<std::string, std::string> m_cgis;
    Option<std::string> m_upload_directory;

    Option<std::string> m_redirect;
};

class ServerConfig
{
public:
    ServerConfig();

    virtual ~ServerConfig()
    {
    }

    virtual Result<int, ConfigError> deserialize(ConfigEntry& from);

    Option<std::string>& server_name()
    {
        return m_server_name;
    }

    Option<struct sockaddr_in>& listen_addr()
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

    std::vector<Location>& locations()
    {
        return m_locations;
    }

    int cgi_timeout()
    {
        return m_cgi_timeout;
    }

    const std::string& error_theme()
    {
        return m_error_theme;
    }

private:
    Option<std::string> m_server_name;
    Option<struct sockaddr_in> m_listen_addr;
    std::map<int, std::string> m_error_pages;

    /* Maximum accepted `Content-Length` */
    size_t m_max_content_length;
    int m_cgi_timeout;

    std::vector<Location> m_locations;
    std::string m_error_theme;
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

    std::vector<ServerConfig>& servers()
    {
        return m_servers;
    }

private:
    std::vector<ServerConfig> m_servers;
};
