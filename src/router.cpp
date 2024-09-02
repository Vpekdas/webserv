#include "router.hpp"
#include "status.hpp"

static Result<File, HttpStatus> _process_default(std::string path)
{
    File file(path);

    if (!file.exists()) return Err<File, HttpStatus>(HttpStatus(404));

    return Ok<File, HttpStatus>(file);
}

Router::Router(std::string root) : m_root(root)
{
}

Result<File, HttpStatus> Router::route(std::string path)
{
    std::string full_path = m_root + "/" + path;
    std::string ext = full_path.substr(full_path.find('.') + 1);

    if (m_processor.count(ext) == 0)
        return _process_default(full_path);
    return m_processor[ext](full_path);
}
