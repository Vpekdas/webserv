#pragma once

#include <cstddef>
#include <map>
#include <string>

#define FILE_BUFFER_SIZE 8192

class File
{
public:
    virtual ~File()
    {
    }

    /*
        Returns the file name.
     */
    virtual std::string& file_name() = 0;

    /*
        Returns `true` if the file exists.
     */
    virtual bool exists() = 0;

    /*
        Returns the size of the file.
     */
    virtual size_t file_size() = 0;

    /*
        Return the mime of the file (e.g. `text/html`, `application/json`).
     */
    virtual std::string mime() = 0;

    /*
        Send the file through a connection fd with `send`.
     */
    virtual void send(int conn) = 0;

    static std::string& mime_from_ext(std::string ext);
    static void _build_mime_table();

protected:
    static std::map<std::string, std::string> mimes;
};

class StreamFile : public File
{
public:
    StreamFile(std::string path);

    virtual std::string& file_name();
    virtual bool exists();
    virtual size_t file_size();
    virtual std::string mime();
    virtual void send(int conn);

private:
    std::string m_path;
};

class StringFile : public File
{
public:
    StringFile(std::string source, std::string mime);

    virtual std::string& file_name();
    virtual bool exists();
    virtual size_t file_size();
    virtual std::string mime();
    virtual void send(int conn);

private:
    std::string m_mime;
    std::string m_content;
    std::string m_fake_name;
};
