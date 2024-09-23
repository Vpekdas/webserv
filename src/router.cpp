#include <cctype>
#include <cstddef>
#include <fstream>
#include <ios>
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
    return Response::ok(200, File::memory(source2, File::mime_from_ext("html")));
}

void Router::_upload_files(Location& loc, Request& req)
{
    size_t i = 0;
    const std::string& body = req.body();

    size_t boundarySize = body.find(SEP);
    std::string boundary = body.substr(i, boundarySize + 2);
    std::string endBoundary = body.substr(i, boundarySize) + "--";

    while (i != std::string::npos)
    {
        size_t headerStart = i + boundary.size();
        size_t contentStart = body.find("\r\n\r\n", headerStart) + 4;

        Request req = Request::parse_part(body.substr(headerStart, contentStart - headerStart)).unwrap();

        std::string& contentDisp = req.get_param("Content-Disposition");
        size_t filenameStart = contentDisp.find("filename=") + 10;
        std::string filename = contentDisp.substr(
            filenameStart, contentDisp.find('\"', contentDisp.find("filename=") + 10) - filenameStart);

        size_t contentEnd = body.find(boundary, contentStart);
        if (contentEnd == std::string::npos)
            contentEnd = body.find(endBoundary, contentStart);

        i = body.find(boundary, contentEnd);

        std::string str = body.substr(contentStart, contentEnd - contentStart);
        std::string filepath = loc.upload_dir().unwrap() + "/" + filename;

        if (filename.empty())
        {
            ws::log << ws::dbg << "File name empty for upload\n";
            continue;
        }

        if (access(filepath.c_str(), F_OK) != -1)
        {
            continue;
        }

        std::ofstream file(filepath.c_str(), std::ios_base::binary | std::ios_base::trunc);
        file.write(str.data(), str.size());

        if (contentEnd == std::string::npos)
        {
            // NOTE: Unreachable if the http request is correctly formatted.
            break;
        }
        if (body.substr(contentEnd, 60) == endBoundary)
        {
            break;
        }
    }
}

Response Router::_route_with_location(Request& req, Location& loc)
{
    Option<std::string> res = loc.redirect();
    if (res.is_some())
    {
        HttpStatus code = 307;

        Response response = Response::ok(code, File::memory("", ""));
        response.add_param("Location", res.unwrap());
        return response;
    }

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

    if (loc.root().is_none())
    {
        return HTTP_ERROR(404, m_config);
    }

    struct stat sb;
    std::string path = loc.root().unwrap() + "/" + req.path().substr(loc.route().size());
    std::string final_path;

    if (stat(path.c_str(), &sb) == -1)
        return HTTP_ERROR(404, m_config);

    if (S_ISDIR(sb.st_mode))
        final_path = path + "/" + loc.default_page().unwrap_or("index.html");
    else
        final_path = path;

    if (req.method() == DELETE)
        return _delete_file(req, loc, path);

    if (stat(final_path.c_str(), &sb) == -1)
        return HTTP_ERROR(404, m_config);
    else if (S_ISDIR(sb.st_mode) && loc.indexing())
        return _directory_listing(req, loc, path);

    if (req.method() == POST && req.get_param("Content-Type").find("multipart/form-data") == 0 &&
        loc.upload_dir().is_some())
    {
        _upload_files(loc, req);
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
        Result<Response, HttpStatus> res = cgi.process(final_path, req, m_config.cgi_timeout(), req.body());
        if (res.is_err())
            return HTTP_ERROR(res.unwrap_err(), m_config);

        return res.unwrap();
    }
    else
    {
        return Response::ok(200, File::stream(final_path));
    }
}

Response Router::_delete_file(Request& req, Location& loc, std::string& path)
{
    (void)req;
    if (access(path.c_str(), F_OK | R_OK) == -1)
        return HTTP_ERROR(404, m_config);
    if (unlink(path.c_str()) == -1)
    {
        ws::log << ws::err << "Cannot delete file " << loc.root().unwrap() << "/" << path << ": " << strerror(errno)
                << "\n ";
        return HTTP_ERROR(500, m_config);
    }
    return HTTP_ERROR(200, m_config);
}

Response Router::route(Request& req)
{
    std::string& path = req.path();

    if (m_config.locations().size() == 0)
        return HTTP_ERROR(404, m_config);

    Location best_match_loc = m_config.locations()[0];
    size_t best_match = -1;

    // std::cout << m_config.locations()[0].route() << ", " << m_config.locations()[1].route() << "\n";

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
        return _route_with_location(req, best_match_loc);
    }
    return HTTP_ERROR(404, m_config);
}
