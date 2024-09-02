#pragma once

#include <cstddef>
#include <map>
#include <string>

#define FILE_BUFFER_SIZE 8192

class File
{
public:
    File();
    File(std::string path);

    /*
        Returns `true` if the file exists.
     */
    bool exists();

    size_t file_size();

    /*
        Return the mime of the file (e.g. `text/html`, `application/json`)
     */
    std::string mime();

    /*
        Send the file through a connection fd with `send`.
     */
    void send(int conn);

private:
    std::string m_path;
    bool m_valid;

    static std::map<std::string, std::string> mimes;

    static void _build_mime_table();
};
