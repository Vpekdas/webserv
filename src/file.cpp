#include "file.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

void __attribute__((constructor)) File::_build_mime_table()
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

File::File() : m_valid(false)
{
}

File::File(std::string path) : m_path(path)
{
    if (access(path.c_str(), F_OK | R_OK) > 0) m_valid = true;
}

bool File::exists()
{
    return m_valid;
}

size_t File::file_size()
{
    struct stat s;

    if ((stat(m_path.c_str(), &s)) == -1)
        return (size_t)-1;
    return s.st_size;
}

std::string File::mime()
{
    std::string ext = m_path.substr(m_path.find('.') + 1);
    return mimes[ext];
}

// TODO:
// To optimize further, small files (like HTML pages) could be stored in RAM.
void File::send(int conn)
{
    if (m_valid) return;

    char buf[FILE_BUFFER_SIZE];
    int fd = open(m_path.c_str(), O_RDONLY);

    if (fd == -1) return;

    int n;

    while ((n = read(fd, buf, FILE_BUFFER_SIZE)) > 0)
    {
        ::send(conn, buf, n, 0);
    }

    // TODO:
    // If there is an error.
    if (n == -1) return;

    close(fd);
}
