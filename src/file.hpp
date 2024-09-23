#pragma once

#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define FILE_BUFFER_SIZE 8192

class File
{
public:
    /*
        Returns the file name.
     */
    std::string& file_name()
    {
        return m_path;
    }

    /*
        Returns `true` if the file exists.
     */
    bool exists()
    {
        return m_in_memory || (access(m_path.c_str(), F_OK | R_OK) != -1);
    }

    /*
        Returns the size of the file.
     */
    size_t file_size()
    {
        if (m_in_memory)
        {
            return m_content.size();
        }

        struct stat sb;

        if (stat(m_path.c_str(), &sb) == -1)
        {
            return 0;
        }

        return sb.st_size;
    }

    /*
        Return the mime of the file (e.g. `text/html`, `application/json`).
     */
    std::string mime()
    {
        if (m_in_memory)
        {
            return m_path;
        }

        return mime_from_ext(m_path.substr(m_path.rfind('.') + 1));
    }

    /*
        Send the file through a connection fd with `send`.
     */
    bool send(int conn)
    {
        if (!m_in_memory)
        {
            if (!exists())
                return false;

            char buf[FILE_BUFFER_SIZE];
            int fd = open(m_path.c_str(), O_RDONLY);

            if (fd == -1)
                return false;

            ssize_t n;

            while ((n = read(fd, buf, FILE_BUFFER_SIZE)) > 0)
            {
                int n2 = ::send(conn, buf, n, 0);
                if (n2 == -1 || n2 == 0)
                {
                    close(fd);
                    return false;
                }
            }

            close(fd);

            if (n <= 0)
                return false;
        }
        else
        {
            ssize_t n;

            n = ::send(conn, m_content.c_str(), file_size(), 0);

            if (n == -1 || n == 0)
                return false;
        }

        return true;
    }

    static File memory(std::string content, std::string mime)
    {
        File file;
        file.m_in_memory = true;
        file.m_content = content;
        file.m_path = mime;
        return file;
    }

    static File stream(std::string path)
    {
        File file;
        file.m_in_memory = false;
        file.m_path = path;
        return file;
    }

    static std::string& mime_from_ext(std::string ext);
    static void _build_mime_table();

private:
    std::string m_path;
    std::string m_content;
    bool m_in_memory;

    static std::map<std::string, std::string> mimes;
};
