# TODOS

- `POST` (still not working with CGI)
- Handle HTTP redirection.
- Add CGI timeout error
- Add connection timeout
- overwrite default error pages

# ERRORS

- Attempting to download a large file prevents the web server from shutting down properly.
- Segfault when attempting to request using HTTPS (e.g. `https://localhost:9999`)
- Payload to large error does not trigger on file upload
- Weird bug when uploading file
- Segfault when trying to use DELETE with a form
<!-- - `411 Length required` on DELETE request -->
