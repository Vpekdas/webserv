#include "router.hpp"
#include "file.hpp"
#include "result.hpp"
#include "smart_pointers.hpp"
#include "status.hpp"

#include <sys/stat.h>
#include <unistd.h>

static Result<File *, HttpStatus> _process_default(std::string path)
{
    struct stat s;
    File *file;

    if (stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
        file = new StreamFile(path + "/index.html");
    else
        file = new StreamFile(path);

    if (file->exists())
        return Result<File *, HttpStatus>(file);
    return Err<File *, HttpStatus>(404);
}

Router::Router(std::string root) : m_root(root)
{
}

Result<File *, HttpStatus> Router::route(std::string path)
{
    std::string full_path = m_root + "/" + path;
    size_t ext_pos = full_path.find('.');
    std::string ext = full_path.substr(full_path.find('.') + 1);

    if (ext_pos == std::string::npos || m_processor.count(ext) == 0)
        return _process_default(full_path);
    return m_processor[ext](full_path);
}
