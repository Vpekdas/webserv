#include "file.hpp"
#include <fcntl.h>
#include <map>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

std::map<std::string, std::string> File::mimes;

void File::_build_mime_table()
{
    // Reference:
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types

    mimes["css"] = "text/css";
    mimes["csv"] = "text/csv";
    mimes["gz"] = "application/gzip"; // macOS + Windows use `application/x-gzip`
    mimes["gif"] = "image/gif";
    mimes["htm"] = "text/html";
    mimes["html"] = "text/html";
    mimes["jpg"] = "image/jpeg";
    mimes["jpeg"] = "image/jpeg";
    mimes["js"] = "text/javascript";
    mimes["json"] = "application/json";
    mimes["mp3"] = "audio/mpeg";
    mimes["mp4"] = "video/mp4";
    mimes["php"] = "application/x-httpd-php";
    mimes["text"] = "text/plain";
}

/*
    StreamFile
 */

StreamFile::StreamFile(std::string path) : m_path(path)
{
}

bool StreamFile::exists()
{
    return access(m_path.c_str(), F_OK | R_OK) == 0;
}

size_t StreamFile::file_size()
{
    struct stat s;

    if ((stat(m_path.c_str(), &s)) != 0)
        return (size_t)-1;
    return s.st_size;
}

std::string StreamFile::mime()
{
    std::string ext = m_path.substr(m_path.find('.') + 1);
    return mimes[ext];
}

// TODO:
// To optimize further, small files (like HTML pages) could be stored in RAM.
void StreamFile::send(int conn)
{
    if (!exists())
        return;

    char buf[FILE_BUFFER_SIZE];
    int fd = open(m_path.c_str(), O_RDONLY);

    if (fd == -1)
        return;

    ssize_t n;

    while ((n = read(fd, buf, FILE_BUFFER_SIZE)) > 0)
    {
        ::send(conn, buf, n, 0);
    }

    close(fd);

    // TODO:
    // If there is an error.
    if (n == -1)
        return;
}

/*
    StringFile
 */

StringFile::StringFile(std::string content, std::string mime) : m_mime(mime), m_content(content)
{
}

bool StringFile::exists()
{
    return true;
}

size_t StringFile::file_size()
{
    return m_content.size();
}

std::string StringFile::mime()
{
    return m_mime;
}

void StringFile::send(int conn)
{
    ssize_t n;

    n = ::send(conn, m_content.c_str(), file_size(), 0);

    // TODO:
    // If there is an error.
    if (n == -1)
        return;
}
