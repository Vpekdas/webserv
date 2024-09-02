#define MAX_EVENTS 5
#define READ_SIZE 1024

#include "../src/colors.hpp"

#include "http/request.hpp"
#include "http/response.hpp"
#include <csignal>

#include <cstdio>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

// bool main_loop = true;

// void sigint(int signum)
// {
//   (void) signum;
//   main_loop = false;
// }

int main() {
  int event_count;
  char read_buffer[READ_SIZE + 1];
  struct epoll_event event, events[MAX_EVENTS];
  int epoll_fd = epoll_create1(0);

  // signal(SIGINT, sigint);

  if (epoll_fd == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": epoll_create1() failed."
              << RESET << std::endl;
    return 1;
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": socket() failed." << RESET
              << std::endl;
    return 1;
  }

  int b = true;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &b, sizeof(int));

  sockaddr_in sockaddr;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = INADDR_ANY;
  sockaddr.sin_port = htons(9999);

  if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": bind() failed." << RESET
              << std::endl;
    return 1;
  }

  if (listen(sockfd, 42) == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": listen() failed." << RESET
              << std::endl;
    return 1;
  }

  socklen_t addrlen = sizeof(sockaddr);
  int connection =
      accept(sockfd, (struct sockaddr *)&sockaddr, (socklen_t *)&addrlen);
  if (connection == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": connection() failed."
              << RESET << std::endl;
    return 1;
  }

  event.events = EPOLLIN | EPOLLET;
  event.data.fd = connection;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection, &event) == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed."
              << RESET << std::endl;
    close(epoll_fd);
    return 1;
  }

  // if (req.get_param("Connection") == "keep-alive")
  //   response.add_param("Connection", "keep-alive");

  while (1) {
    std::cout << YELLOW << "Polling for input..." << RESET << std::endl;
    event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    std::cout << GREEN << event_count << " ready events." << RESET << std::endl;
    for (int i = 0; i < event_count; i++) {
      if ((events[i].events & EPOLLIN) == EPOLLIN) {
        std::cout << CYAN << "Reading file descriptor: " << events[i].data.fd
                  << RESET << std::endl;

        int bytes_read = recv(events[i].data.fd, read_buffer, READ_SIZE, 0);
        read_buffer[bytes_read] = '\0';

        std::cout << CYAN << "Reading message: " << read_buffer << RESET
                  << std::endl;
        std::cout << PURPLE << "bytes read: " << bytes_read << RESET
                  << std::endl;

        struct epoll_event event;
        event.events = EPOLLOUT | EPOLLET;
        event.data.fd = events[i].data.fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event) ==
            -1) {
          std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed."
                    << RESET << std::endl;
          close(events[i].data.fd);
          continue;
        }
      }

      else if ((events[i].events & EPOLLOUT) == EPOLLOUT) {
        Response response = Response::httpcat(401);
        response.add_param("Connection", "keep-alive");

        std::string body = response.encode();

        std::cout << YELLOW << "Sending message..." << RESET << std::endl;

        if (send(connection, body.data(), body.size(), 0) == -1) {
          std::cerr << NRED << strerror(errno) << RED << ": send() failed."
                    << RESET << std::endl;
        }

        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = events[i].data.fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &event) ==
            -1) {
          std::cerr << NRED << strerror(errno) << RED << ": epoll_ctl() failed."
                    << RESET << std::endl;
          close(events[i].data.fd);
          continue;
        }
      }
    }
  }

  if (close(connection) == -1) {
    std::cerr << NRED << strerror(errno) << RED
              << ": close() connection failed." << RESET << std::endl;
    return 1;
  }

  if (close(epoll_fd) == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": close() epoll_fd failed."
              << RESET << std::endl;
    return 1;
  }

  if (close(sockfd) == -1) {
    std::cerr << NRED << strerror(errno) << RED << ": close() sockfd failed."
              << RESET << std::endl;
    return 1;
  }

  return 0;
}
