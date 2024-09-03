#include "WebServ.hpp"

int main()
{
    File::_build_mime_table();

    int event_count;
    char read_buffer[READ_SIZE];
    struct epoll_event events[MAX_EVENTS];

    Router router("www");

    // Method created
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == FAILURE)
    {
        std::cerr << NRED << strerror(errno) << RED << ": epoll_create1() failed." << RESET << std::endl;
        return FAILURE;
    }

    // Method created
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": socket() failed." << RESET << std::endl;
        return 1;
    }

    // Method created
    int b = true;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &b, sizeof(int));

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(9999);

    // Ensure the server can accept incoming connections by binding the socket to
    // the specified IP address and port. This step is crucial for the server to
    // listen for and accept client requests on the designated network interface.
    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": bind() failed." << RESET << std::endl;
        return 1;
    }

    // Prepare the socket to accept incoming connection requests by setting it to
    // a listening state. This is essential for the server to handle multiple
    // client connections concurrently.
    if (listen(sockfd, 42) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": listen() failed." << RESET << std::endl;
        return 1;
    }

    // Accept an incoming connection to establish communication with a client.
    // This step is crucial for the server to handle client requests and provide
    // responses.
    socklen_t addrlen = sizeof(sockaddr);
    int connection = accept(sockfd, (struct sockaddr *)&sockaddr, (socklen_t *)&addrlen);
    if (connection == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": connection() failed." << RESET << std::endl;
        return 1;
    }

    // Configure the epoll event to monitor the new connection for incoming data
    // and edge-triggered events. This setup is essential for efficiently
    // detecting and handling I/O events on the connection, ensuring the server
    // can respond promptly to client requests.
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = connection;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection, &event) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
        close(epoll_fd);
        return 1;
    }

    std::string req_str;

    while (1)
    {
        std::cout << YELLOW << "Polling for input..." << RESET << std::endl;
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        std::cout << GREEN << event_count << " ready events." << RESET << std::endl;
        for (int i = 0; i < event_count; i++)
        {

            // Check if the current event indicates that there is data to read on the
            // file descriptor.
            // This is crucial for processing incoming data from the client, ensuring
            // the server can handle read operations when data is available.
            if ((events[i].events & EPOLLIN) == EPOLLIN)
            {
                std::cout << CYAN << "Reading file descriptor: " << events[i].data.fd << RESET << std::endl;

                int bytes_read = recv(events[i].data.fd, read_buffer, READ_SIZE, 0);

                req_str.append(read_buffer, bytes_read);

                // Modify the event to monitor the file descriptor for outgoing data.
                // This is necessary to prepare the server to send a response back to
                // the client after reading the incoming data.
                struct epoll_event event;
                event.events = EPOLLOUT | EPOLLET;
                event.data.fd = events[i].data.fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event) == -1)
                {
                    std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
                    close(events[i].data.fd);
                    continue;
                }
            }

            // Check if the current event indicates that the file descriptor is ready
            // for writing. This is essential for sending data to the client, ensuring
            // the server can handle write operations when the socket is ready.
            else if ((events[i].events & EPOLLOUT) == EPOLLOUT)
            {
                Request req = Request::parse(req_str).unwrap();
                req_str.clear();
                Result<File *, HttpStatus> res = router.route(req.path());

                Response response;

                if (res.is_err())
                    response = Response::httpcat(res.unwrap_err());
                else
                    response = Response::ok(200, res.unwrap());

                response.add_param("Connection", "keep-alive");
                response.send(connection);

                // Modify the event to monitor the file descriptor for outgoing data.
                // This is necessary to prepare the server to send a response back to
                // the client after reading the incoming data.
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = events[i].data.fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event) == -1)
                {
                    std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed." << RESET << std::endl;
                    close(events[i].data.fd);
                    continue;
                }
            }
        }
    }

    if (close(connection) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": close() connection failed." << RESET << std::endl;
        return 1;
    }

    if (close(epoll_fd) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": close() epoll_fd failed." << RESET << std::endl;
        return 1;
    }

    if (close(sockfd) == -1)
    {
        std::cerr << NRED << strerror(errno) << RED << ": close() sockfd failed." << RESET << std::endl;
        return 1;
    }

    return 0;
}
