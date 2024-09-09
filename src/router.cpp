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
#include <dirent.h>
#include <time.h>

// clang-format off
static const std::string source =
"<!DOCTYPE html>" SEP
"<html lang='en'>" SEP
"    <head>" SEP
"        <meta charset='UTF-8' />" SEP
"        <meta name='viewport' content='width=device-width, initial-scale=1.0' />" SEP
"        <link rel='stylesheet' href='/style.css' />"
"        <title>Index</title>"
"    </head>" SEP
"    <body>" SEP
        "<table>" SEP
            "<th>" SEP
"		 <span>Index of #path</span>"
                "<tr>"
                    "<td><b>Name</b></td>" SEP
                    "<td><b>Size</b></td>" SEP
                    "<td><b>Last Modified</b></td>" SEP
                "</tr>"
            "</th>" SEP
            "<tbody>"
                "#replace" SEP
            "</tbody>"
        "</table>" SEP
"</body>" SEP
"</html>" SEP;
// clang-format on

Router::Router(ServerConfig config) : m_config(config)
{
}

Response Router::_directory_listing(Request& req, Location& loc, std::string path)
{
    DIR *dir;
    struct dirent *entry;
    (void)req;
    (void)loc;

    if ((dir = opendir(path.c_str())) == NULL)
    {
        ws::log << ws::err << "opendir() failed: " << strerror(errno) << "\n";
        return Response::httpcat(404);
    }

    std::string indexing;

    struct stat sb;

    while ((entry = readdir(dir)) != NULL)
    {
        indexing += "<tr> <td> <a href='";
        indexing += loc.route().substr(1);
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

Response Router::_route_with_location(Request& req, Location& loc)
{
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
        return HttpStatus(405); // Method not allowed

    struct stat sb;
    std::string path = loc.root() + "/" + req.path().substr(loc.route().size());
    std::string final_path;

    if (stat(path.c_str(), &sb) != -1 && S_ISDIR(sb.st_mode))
        final_path = path + "/" + loc.default_page();
    else
        final_path = path;

    if (access(final_path.c_str(), F_OK | R_OK) == -1)
    {
        if (loc.indexing() && S_ISDIR(sb.st_mode))
            return _directory_listing(req, loc, path);
        else
            return Response::httpcat(404);
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
        Result<std::string, HttpStatus> res = cgi.process(final_path, req);
        if (res.is_err())
            return Response::httpcat(res.unwrap_err());

        return Response::from_cgi(200, res.unwrap());
    }
    else
    {
        return Response::ok(200, new StreamFile(final_path));
    }
}

Response Router::route(Request& req)
{
    std::string path = req.path();

    for (std::vector<Location>::iterator it = m_config.locations().begin(); it != m_config.locations().end(); it++)
    {
        Location location = *it;
        if (path.find(location.route()) == 0)
            return _route_with_location(req, location);
    }

    return HttpStatus(404);
}
