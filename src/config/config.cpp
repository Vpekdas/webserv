#include "config/config.hpp"
#include "config/parser.hpp"
#include "result.hpp"
#include "string.hpp"
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <vector>

template <typename T>
static std::vector<T> _array_to_vec(T array[], size_t size)
{
    std::vector<T> vec;

    vec.reserve(size);
    for (size_t i = 0; i < size; i++)
    {
        vec.push_back(array[i]);
    }

    return vec;
}

Location::Location() : m_enable_indexing(true)
{
}

Result<int, ConfigError> Location::deserialize(ConfigEntry& from)
{
    m_route = from.args()[1].str();

    for (size_t i = 0; i < from.children().size(); i++)
    {
        ConfigEntry& entry = from.children()[i];
        Token& token_name = entry.args()[0];
        std::string name = token_name.content();

        if (name == "methods" && entry.is_inline() && entry.args().size() == 2 &&
            entry.args()[1].type() == TOKEN_IDENTIFIER)
        {
            std::vector<std::string> methods = split(entry.args()[1].str(), ',');

            for (std::vector<std::string>::iterator it = methods.begin(); it != methods.end(); it++)
            {
                if (*it == "GET")
                    m_methods.push_back(GET);
                else if (*it == "POST")
                    m_methods.push_back(POST);
                else if (*it == "DELETE")
                    m_methods.push_back(DELETE);
                else
                    return ConfigError::invalid_method(entry.source(), entry.args()[1]);
            }
        }
        else if (name == "root" && entry.is_inline() && entry.args().size() == 2 &&
                 entry.args()[1].type() == TOKEN_STRING)
        {
            m_root = entry.args()[1].str();
        }
        else if (name == "index" && entry.is_inline() && entry.args().size() == 2 &&
                 entry.args()[1].type() == TOKEN_IDENTIFIER)
        {
            std::string indexing = entry.args()[1].content();
            if (indexing == "enable")
                m_enable_indexing = true;
            else if (indexing == "disable")
                m_enable_indexing = false;
        }
        else if (name == "default" && entry.is_inline() && entry.args().size() == 2 &&
                 entry.args()[1].type() == TOKEN_STRING)
        {
            m_default = entry.args()[1].str();
        }
        else if (name == "cgi" && entry.is_inline() && entry.args().size() == 3 &&
                 entry.args()[1].type() == TOKEN_STRING && entry.args()[2].type() == TOKEN_STRING)
        {
            m_cgis[entry.args()[1].str()] = entry.args()[2].str();
        }
        else if (name == "upload_dir" && entry.args().size() == 2 && entry.args()[1].type() == TOKEN_STRING)
        {
            m_upload_directory = entry.args()[1].str();
        }
        else if (name == "redirect" && entry.args().size() == 2 && entry.args()[1].type() == TOKEN_STRING)
        {
            m_redirect = Some(entry.args()[1].str());
        }
        else
        {
            std::string entries[] = {"methods", "root", "index", "default", "cgi", "upload_dir", "redirect"};
            return ConfigError::unknown_entry(entry.source(), token_name,
                                              _array_to_vec(entries, sizeof(entries) / sizeof(std::string)));
        }
    }
    return 0;
}

ServerConfig::ServerConfig() : m_cgi_timeout(1000)
{
}

Result<int, ConfigError> ServerConfig::deserialize(ConfigEntry& from)
{
    for (size_t i = 0; i < from.children().size(); i++)
    {
        ConfigEntry& entry = from.children()[i];
        Token& token_name = entry.args()[0];
        std::string name = token_name.content();

        if (name == "server_name" && entry.is_inline() && entry.args().size() == 2 &&
            entry.args()[1].type() == TOKEN_STRING)
            m_server_name = entry.args()[1].str();
        else if (name == "listen" && entry.is_inline() && entry.args().size() == 2 &&
                 entry.args()[1].type() == TOKEN_STRING)
        {
            if (entry.args()[1].str().empty())
                return ConfigError::address(entry.source(), entry.args()[1]);

            std::vector<std::string> parts = split(entry.args()[1].str(), ':');
            std::vector<std::string> ip_parts = split(parts[0], '.');

            struct sockaddr_in listen_addr;
            listen_addr.sin_family = AF_INET;

            int port = std::atoi(parts[1].c_str());
            listen_addr.sin_port = htons(port);
            listen_addr.sin_addr.s_addr =
                (in_addr_t)(std::atoi(ip_parts[0].c_str()) << 24 | std::atoi(ip_parts[1].c_str()) << 16 |
                            std::atoi(ip_parts[2].c_str()) << 8 | std::atoi(ip_parts[3].c_str()));
            m_listen_addr = listen_addr;
        }
        else if (name == "error_page" && entry.is_inline() && entry.args().size() == 3 &&
                 entry.args()[1].type() == TOKEN_NUMBER && entry.args()[2].type() == TOKEN_STRING)
        {
            int status_code = entry.args()[1].number();

            if (status_code < 400 || status_code >= 600)
                return ConfigError::not_in_range(entry.source(), entry.args()[1], 400, 599);

            std::string page = entry.args()[2].str();
            m_error_pages[status_code] = page;
        }
        else if (name == "max_content_length" && entry.is_inline() && entry.args().size() == 2 &&
                 entry.args()[1].type() == TOKEN_NUMBER)
        {
            m_max_content_length = entry.args()[1].number();
        }
        else if (name == "location" /*&& !entry.is_inline()*/ && entry.args().size() == 2 &&
                 entry.args()[1].type() == TOKEN_STRING)
        {
            Location location;
            Result<int, ConfigError> res = location.deserialize(entry);
            EXPECT_OK(int, ConfigError, res);
            m_locations.push_back(location);
        }
        else if (name == "cgi_timeout" && entry.args().size() == 2 && entry.args()[1].type() == TOKEN_NUMBER)
        {
            m_cgi_timeout = entry.args()[1].number();
        }
        else if (name == "error_theme" && entry.args().size() == 2 && entry.args()[1].type() == TOKEN_STRING)
        {
            m_error_theme = entry.args()[1].str();
        }
        else
        {
            std::string entries[] = {"server_name",        "listen",   "error_page",
                                     "max_content_length", "location", "cgi_timeout"};
            return ConfigError::unknown_entry(entry.source(), token_name,
                                              _array_to_vec(entries, sizeof(entries) / sizeof(std::string)));
        }
    }
    return 0;
}

Config::Config()
{
}

Result<int, ConfigError> Config::load_from_file(std::string filepath)
{
    ConfigParser parser;
    EXPECT_OK(int, ConfigError, parser.parse(filepath));
    EXPECT_OK(int, ConfigError, deserialize(parser.root()));
    return 0;
}

Result<int, ConfigError> Config::deserialize(ConfigEntry& from)
{
    for (size_t i = 0; i < from.children().size(); i++)
    {
        ConfigEntry& entry = from.children()[i];
        Token& entry_name = entry.args()[0];

        if (entry_name.content() != "server")
            return ConfigError::mismatch_entry(entry.source(), entry_name, "server", std::vector<Arg>());

        ServerConfig server;
        Result<int, ConfigError> res = server.deserialize(entry);
        EXPECT_OK(int, ConfigError, res);

        m_servers.push_back(server);
    }

    return 0;
}
