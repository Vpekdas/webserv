# Webserv

## Table of Contents
1. [Description](#description)
2. [Installation](#installation)
3. [Run](#run)
4. [Credits](#credits)
5. [Contributing](#contributing)

## Description

Webserv is a project from the 42 Common Core curriculum. The goal is to replicate the features of a web server, such as Nginx. As part of the 42 curriculum, we are limited to using only C++98 features and must handle I/O operations using tools like epoll. One of the key restrictions is that we cannot use read() function calls unless there are active epoll events, ensuring efficiency.

## Screenshot

1. Our beautifully designed file upload page. It features two forms on a clean, minimalist layout :)
<img width="1440" alt="Screenshot 2024-11-03 at 19 06 06" src="https://github.com/user-attachments/assets/b19ab23e-61e7-4295-8bb8-33ea350af322">

2. Directory listing: Firefox front-end developers should hire me! I replicated the directory listing page with the same level of detail (almost) as seen in the Firefox browser.
<img width="1440" alt="Screenshot 2024-11-03 at 19 06 56" src="https://github.com/user-attachments/assets/f1c73de9-4ca8-4303-9b2b-da52c2e99697">

3. Two of our custom error pages (featuring cats, which I think are better than the rest of the error themes).
<img width="1440" alt="Screenshot 2024-11-03 at 19 10 55" src="https://github.com/user-attachments/assets/05cfe889-638a-41a3-b71d-caadf5f2be35">
<img width="1440" alt="Screenshot 2024-11-03 at 19 11 03" src="https://github.com/user-attachments/assets/3c7a6de3-0430-4692-bac9-9d2232a1b3d6">

4. Of course, I won't miss a chance to showcase my portfolio!
<img width="1440" alt="Screenshot 2024-11-03 at 19 09 01" src="https://github.com/user-attachments/assets/22c8b1b9-31b8-47bc-8355-cd18934c6e60">

### Purpose

Webserv can be used as a web server, similar to Nginx, although it does not support as many features. However, it handles the following core functionalities:

- Compatible with Firefox browser.
- Compliant with HTTP/1.1 standards.
- Customizable HTTP responses (e.g., cat, pizza, garden, or dog).
- Default error pages.
- Capable of serving fully static websites.
- File upload handling.
- Supports GET, POST, and DELETE methods.
- Stress-tested for 100% availability using Siege.
- Ability to listen on multiple ports.
- Supports CGI scripts in Bash, PHP, and Python.
- Directory listing.
- HTTP redirection.

### Technologies used

- C++
- Make

### Challenges and Future Features

The main challenges were understanding I/O operations, selecting the appropriate functions to handle them, and grasping how sockets work in C. Here are the features I was in charge of:

- Handling all epoll event loops: Setting up the listening loop and adjusting the structure to use `EPOLLIN` or `EPOLLOUT` to determine whether we are still receiving a request or are ready to send a response.
- Creating, binding, and listening to the socket: Establishing the socket and ensuring it was ready to handle incoming HTTP requests.
- Designing the directory listing page: Inspired by the directory listing in the Firefox browser, ensuring a user-friendly view of available files.
- Implementing file uploads: Parsing multipart/form-data to handle multiple file uploads, ensuring the server processes and stores files correctly.

I'm not planning to add new features.

## Installation

- Ensure you have a C compiler installed, such as [Clang](https://clang.llvm.org/) or [GCC](https://gcc.gnu.org/)
```bash
clang --version
gcc --version
```
- Ensure you have installed [Make](https://www.gnu.org/software/make/) to build the project
```bash
make --version
```

This project is designed to run only on Unix-based systems since it uses Epoll for I/O handling. However, I developed it on macOS using [OrbStack](https://orbstack.dev/). OrbStack allows you to create lightweight containers to build and run the project, making it possible to work on macOS even though the project is intended for Unix systems.

## Run

1. Build the project:
```bash
make
```

2. Run the project with a preconfigured config or create your own configuration by following the provided templates:
```bash
./webserv config
```

3. Access the server:

- Navigate to localhost:port/index.html if it's specified in the root directory.
- For other pages or scripts, visit localhost:port/another_page or any other route defined in your configuration.

## Credits

Special thanks to the following individuals and resources for their contributions to this project:

[FirePh0enix](https://github.com/FirePh0enix): An outstanding teammate who was in charge of :
* Designing the architecture of our web server, including the design of most of the classes.
* Parsing the configuration files, ensuring there were no errors.
* Parsing the HTTP requests to populate our internal classes and structures, allowing me to access the necessary variables for my part.
* Managing error handling, including customized error pages, setting up routers, multiple servers, and the logger.
* Managing the MIME types table.

## Contributing

To report issues, please create an issue here:  [issue tracker](https://github.com/Vpekdas/webserv/issues).

To contribute, follow these steps:

1. **Fork the repository**: Start by forking the repository to your own GitHub account.

2. **Clone the repository**: Clone the forked repository to your local machine.
```bash
git clone https://github.com/Vpekdas/webserv
```

3. **Create a new branch**: Create a new branch for each feature or bug fix you're working on.
```bash
git checkout -b your-branch-name
```

4. **Commit your changes**: Commit your changes.
```bash
git commit -m "Your commit message"
```

5. **Push your changes**: Push your changes to your forked repository on GitHub.
```bash
git push origin your-branch-name
```

6. **Create a pull request**: Go to your forked repository on GitHub and create a new pull request against the master branch.
