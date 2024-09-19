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
    mimes["png"] = "image/jpeg";
    mimes["js"] = "text/javascript";
    mimes["json"] = "application/json";
    mimes["mp3"] = "audio/mpeg";
    mimes["mp4"] = "video/mp4";
    mimes["png"] = "image/png";
    mimes["php"] = "application/x-httpd-php";
    mimes["text"] = "text/plain";
}

std::string& File::mime_from_ext(std::string ext)
{
    return mimes[ext];
}
