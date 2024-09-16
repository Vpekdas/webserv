#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "cgi/cgi.hpp"
#include "config/config.hpp"
#include "file.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/status.hpp"
#include "logger.hpp"
#include "result.hpp"
#include "router.hpp"
#include "string.hpp"
#include "webserv.hpp"
#include <cstdio>
#include <dirent.h>
#include <time.h>

// clang-format off
static const std::string source =
"<!DOCTYPE html>" SEP
"<html lang='en'>" SEP
"    <head>" SEP
"        <meta charset='UTF-8' />" SEP
"        <meta name='viewport' content='width=device-width, initial-scale=1.0' />" SEP
"        <title>Index of #path</title>" SEP
"        <style>" SEP
"body {" SEP
"    background-color: #353535;" SEP
"    font-family: sans-serif;" SEP
"}" SEP
"table {" SEP
"    margin-top: 60px;" SEP
"    margin-left: auto;" SEP
"    margin-right: auto;" SEP
"    background-color: #2b2a33;" SEP
"    width: 50%;" SEP
"    border: 1px solid black;" SEP
"    border-radius: 10px;" SEP
"}" SEP
"td {" SEP
"    color: white;" SEP
"    text-align: center;" SEP
"}" SEP
"a:visited {" SEP
"    color: #fba9fb;" SEP
"}" SEP
"a {" SEP
"    color: #8181ea;" SEP
"}" SEP
"span {" SEP
"    border-bottom: 1px solid grey;" SEP
"    color: white;" SEP
"	float: left;" SEP
"}" SEP
"        </style>"
"    </head>" SEP
"    <body>" SEP
        "<table>" SEP
            "<th>" SEP
"		 <span>Index of #path</span>" SEP
                "<tr>" SEP
                    "<td><b>Name</b></td>" SEP
                    "<td><b>Size</b></td>" SEP
                    "<td><b>Last Modified</b></td>" SEP
                "</tr>" SEP
            "</th>" SEP
            "<tbody>" SEP
                "#replace" SEP
            "</tbody>" SEP
        "</table>" SEP
"</body>" SEP
"</html>" SEP;
// clang-format on

Router::Router(ServerConfig config) : m_config(config)
{
}

Response Router::_directory_listing(Request& req, Location& loc, std::string& path)
{
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path.c_str())) == NULL)
    {
        ws::log << ws::err << "opendir() failed: " << strerror(errno) << "\n";
        return HTTP_ERROR(404, m_config);
    }

    std::string indexing;

    struct stat sb;

    while ((entry = readdir(dir)) != NULL)
    {
        indexing += "<tr> <td> <a href='";
        indexing += loc.route().substr(1);

        if (loc.route()[loc.route().size() - 1] != '/')
            indexing += '/';

        indexing += entry->d_name;

        std::string filepath = path + "/" + entry->d_name;

        if (stat(filepath.c_str(), &sb) == -1)
        {
            continue;
        }

        indexing += "'>";
        indexing += entry->d_name;
        indexing += "</a> </td>";
        indexing += "<td> ";

        if (S_ISDIR(sb.st_mode))
            indexing += "";
        else if (sb.st_size < 1024)
            indexing += to_string(sb.st_size) + " B";
        else if (sb.st_size < 1024 * 1024)
            indexing += to_string(sb.st_size / (1024)) + " KiB";
        else if (sb.st_size < 1024 * 1024 * 1024)
            indexing += to_string(sb.st_size / (1024 * 1024)) + " MiB";
        else
            indexing += to_string(sb.st_size / (1024 * 1024 * 1024)) + " GiB";

        indexing += "</td>";
        indexing += "<td>";
        indexing += ctime(&sb.st_mtime);
        indexing += "</td> </tr>";
    }

    std::string source2 = source;
    replace_all(source2, "#replace", indexing);
    replace_all(source2, "#path", req.path());

    closedir(dir);
    return Response::ok(200, new StringFile(source2, File::mime_from_ext("html")));
}

Response Router::_route_with_location(Request& req, Location& loc, std::string& req_str)
{
    // FIXME: THIS IS BROKEN (ALSO FUCK BROWSER CACHE)

    // Option<std::string> res = loc.redirect();
    // if (res.is_some())
    // {
    //     HttpStatus code;

    //     if (req.method() == GET || req.method() == HEAD)
    //     {
    //         code = 301;
    //     }
    //     else if (req.method() == POST)
    //     {
    //         code = 308;
    //     }

    //     Response response = Response::ok(code, new StringFile("", ""));
    //     response.add_param("Location", res.unwrap());
    //     return response;
    // }

    size_t n = 0;
    for (std::vector<Method>::iterator it = loc.methods().begin(); it != loc.methods().end(); it++)
    {
        if (*it == req.method())
        {
            n = 1;
            break;
        }
    }

    if (n == 0)
        return HTTP_ERROR(405, m_config); // Method not allowed

    struct stat sb;
    std::string path = loc.root() + "/" + req.path().substr(loc.route().size());
    std::string final_path;

    if (stat(path.c_str(), &sb) == -1)
        return HTTP_ERROR(404, m_config);

    if (S_ISDIR(sb.st_mode))
        final_path = path + "/" + loc.default_page();
    else
        final_path = path;

    if (req.method() == DELETE)
        return _delete_file(req, loc, path);

    if (access(final_path.c_str(), F_OK | R_OK) == -1)
    {
        if (S_ISDIR(sb.st_mode) && loc.indexing())
            return _directory_listing(req, loc, path);
        else
            return HTTP_ERROR(404, m_config);
    }

    if (req.method() == POST && req.get_param("Content-Type").find("multipart/form-data") == 0)
    {
        size_t pos = req_str.find(SEP SEP);

        std::string header;
        std::string body;

        if (pos == std::string::npos)
        {
            header = req_str;
        }
        else
        {
            header = req_str.substr(0, pos + 2);
            body = req_str.substr(pos + 4);
        }

        Request part_req = Request::parse_part(header).unwrap();

        // TODO: Maybe try to protect the find below ?
        int fileNamePos = header.find("filename=") + 10;
        std::string fileName;

        while (header[fileNamePos] && header[fileNamePos] != '"')
        {
            fileName += header[fileNamePos];
            fileNamePos++;
        }

        std::ofstream file((loc.upload_dir() + "/" + fileName).c_str(), std::ios::binary);
        if (file.is_open())
        {
            file.write(body.c_str(), body.size());
            file.close();
        }
        else
        {
            std::cout << CYAN << "cannot open file" << RESET << std::endl;
        }
    }

    std::string ext = final_path.substr(final_path.rfind('.') + 1);
    n = 0;

    for (std::map<std::string, std::string>::iterator it = loc.cgis().begin(); it != loc.cgis().end(); it++)
    {
        if (it->first == ext)
        {
            n = 1;
            break;
        }
    }

    if (n > 0)
    {
        CGI cgi(loc.cgis()[ext]);
        Result<std::string, HttpStatus> res = cgi.process(final_path, req, m_config.cgi_timeout());
        if (res.is_err())
            return HTTP_ERROR(res.unwrap_err(), m_config);

        return Response::from_cgi(200, res.unwrap());
    }
    else
    {
        return Response::ok(200, new StreamFile(final_path));
    }
}

Response Router::_delete_file(Request& req, Location& loc, std::string& path)
{
    (void)req;
    if (access(path.c_str(), F_OK | R_OK) == -1)
        return HTTP_ERROR(404, m_config);
    if (unlink(path.c_str()) == -1)
    {
        ws::log << ws::err << "Cannot delete file " << loc.root() << "/" << path << ": " << strerror(errno) << "\n ";
        return HTTP_ERROR(500, m_config);
    }
    return HTTP_ERROR(200, m_config);
}

Response Router::route(Request& req, std::string& req_str)
{
    std::string path = req.path();

    if (m_config.locations().size() == 0)
        return HTTP_ERROR(404, m_config);

    Location& best_match_loc = m_config.locations()[0];
    size_t best_match = -1;

    for (std::vector<Location>::iterator it = m_config.locations().begin(); it != m_config.locations().end(); it++)
    {
        Location& location = *it;
        if (path.find(location.route()) == 0 && (best_match == (size_t)-1 || location.route().size() > best_match))
        {
            best_match = location.route().size();
            best_match_loc = location;
        }
    }

    if (path.find(best_match_loc.route()) == 0)
    {
        return _route_with_location(req, best_match_loc, req_str);
    }
    return HTTP_ERROR(404, m_config);
}
