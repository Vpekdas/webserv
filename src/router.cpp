#include "router.hpp"
#include "cgi/cgi.hpp"
#include "config/config.hpp"
#include "file.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/status.hpp"
#include "logger.hpp"
#include "result.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

Router::Router(ServerConfig config) : m_config(config)
{
}

Result<Response, HttpStatus> Router::_route_with_location(Request& req, Location& loc)
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
        return HttpStatus(405);

    ws::log << ws::dbg << loc.route() << "\n";

    struct stat sb;
    std::string path = loc.root() + "/" + req.path().substr(loc.route().size());
    std::string final_path;

    if (stat(path.c_str(), &sb) != -1 && S_ISDIR(sb.st_mode))
        final_path = path + "/" + loc.default_page();
    else
        final_path = path;

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

    if (access(final_path.c_str(), F_OK | R_OK) == -1)
        return HttpStatus(404);

    if (n > 0)
    {
        CGI cgi(loc.cgis()[ext]);
        Result<std::string, HttpStatus> res = cgi.process(final_path, req);
        EXPECT_OK(Response, HttpStatus, res);

        return Response::from_cgi(200, res.unwrap());
    }
    else
    {
        return Response::ok(200, new StreamFile(final_path));
    }
}

Result<Response, HttpStatus> Router::route(Request& req)
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

// Result<Response, HttpStatus> Router::route(Request& req)
// {
//     std::string full_path = m_root + "/" + req.path();
//     std::string ext = full_path.substr(full_path.rfind('.') + 1);

//     struct stat sb;

//     // A special case: append `index.html` or `index.php` if `path` is a directory.
//     // TODO: If both exists, which one has the priority ?
//     if ((stat(full_path.c_str(), &sb) != -1) && S_ISDIR(sb.st_mode))
//     {
//         std::string new_path = full_path + "/index.php";

//         if (access(new_path.c_str(), F_OK | R_OK) != -1)
//             full_path = new_path;
//         else
//             full_path = full_path + "/index.html";
//     }

//     // Either go through a CGI or send the file.

//     if (access(full_path.c_str(), F_OK | R_OK) == -1)
//         return Err<Response, HttpStatus>(404);

//     Response response;

//     if (m_cgis.count(ext) > 0)
//     {
//         Result<std::string, HttpStatus> res = m_cgis[ext].process(full_path, req);
//         if (res.is_err())
//             return Err<Response, HttpStatus>(res.unwrap_err());

//         return Response::from_cgi(200, res.unwrap());
//     }
//     else
//     {
//         File *file = new StreamFile(full_path);
//         if (file->exists())
//             return Response::ok(200, file);
//         else
//             return HttpStatus(404);
//     }
// }
