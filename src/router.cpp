#include "router.hpp"
#include "cgi/cgi.hpp"
#include "file.hpp"
#include "http/request.hpp"
#include "http/status.hpp"
#include "result.hpp"
#include "smart_pointers.hpp"

#include <sys/stat.h>
#include <unistd.h>

Router::Router(std::string root) : m_root(root)
{
    m_cgis["php"] = CGI("/usr/bin/php-cgi");
}

Result<File *, HttpStatus> Router::route(std::string path)
{
    std::string full_path = m_root + "/" + path;
    std::string ext = full_path.substr(full_path.rfind('.') + 1);

    struct stat sb;

    // A special case: append `index.html` or `index.php` if `path` is a directory.
    // TODO: If both exists, which one has the priority ?
    if ((stat(full_path.c_str(), &sb) != -1) && S_ISDIR(sb.st_mode))
    {
        std::string new_path = full_path + "/index.php";

        if (access(new_path.c_str(), F_OK | R_OK) != -1)
            full_path = new_path;
        else
            full_path = full_path + "/index.html";
    }

    // Either go through a CGI or send the file.

    if (access(full_path.c_str(), F_OK | R_OK) == -1)
        return Err<File *, HttpStatus>(404);

    File *file = NULL;

    if (m_cgis.count(ext) > 0)
    {
        Result<std::string, HttpStatus> res = m_cgis[ext].process(full_path);
        if (res.is_err())
            return Err<File *>(res.unwrap_err());
        std::string s = res.unwrap();
        s = s.substr(s.find(SEP SEP) + 2);

        file = new StringFile(s, File::mime_from_ext("html"));
    }
    else
    {
        file = new StreamFile(full_path);
    }

    return Ok<File *, HttpStatus>(file);
}
