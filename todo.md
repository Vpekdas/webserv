# TODOS

- `Content-Length` must be set in the header or else we dont read anything.
- `Transfer-Encoding: chunked` indicate chuncked request
- `POST`
- `DELETE`
- Upload files.
- Handle HTTP redirection.

# ERRORS

- Attempting to download a large file prevents the web server from shutting down properly.
- Conditional Jump when trying to fetch /favicon.ico (Ctrl+Shift+R to reload the cache)
- Segfault when attempting to request using HTTPS (e.g. `https://localhost:9999`)
- Payload to large error does not trigger on file upload
- Weird bug when uploading file
- Segfault when trying to use DELETE with a form
