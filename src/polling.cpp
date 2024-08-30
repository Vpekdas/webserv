#include "http/request.hpp"
#include "http/response.hpp"
#include <csignal>
#define MAX_EVENTS 5
#define READ_SIZE 1024

#include "../src/colors.hpp"

#include <cstdio>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <iostream>

bool main_loop = true;

void sigint(int signum)
{
  (void) signum;
  main_loop = false;
}

int main() {
  int event_count;
  size_t bytes_read;
  char read_buffer[READ_SIZE + 1];
  struct epoll_event event, events[MAX_EVENTS];
  int epoll_fd = epoll_create1(0);

  // signal(SIGINT, sigint);

  if (epoll_fd == -1) {
    perror(RED "epoll_create1() failed\n" RESET);
    return 1;
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror(RED "socket() failed\n" RESET);
    return 1;
  }

  int b = true;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &b, sizeof(int));

  sockaddr_in sockaddr;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = INADDR_ANY;
  sockaddr.sin_port = htons(9999);

  if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
    perror(RED "bind failed\n" RESET);
    return 1;
  }

  event.events = EPOLLIN | EPOLLET;
  event.data.fd = 0;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
    // perror(RED "epoll_ctl() failed\n" RESET);
    std::cerr << strerror(errno) << ": " RED "epoll_ctl() failed\n" RESET;
    close(epoll_fd);
    return 1;
  }

  if (listen(sockfd, 42) == -1) {
    perror(RED "listen failed\n" RESET);
    return 1;
  }

  socklen_t addrlen = sizeof(sockaddr);
  int connection =
      accept(sockfd, (struct sockaddr *)&sockaddr, (socklen_t *)&addrlen);
  if (connection == -1) {
    perror(RED "accept failed\n" RESET);
    return 1;
  }

  char buffer[1024];

  read(connection, buffer, 1024);
  std::cout << CYAN << "The message was " << buffer << RESET << std::endl;

  Request req = Request::parse(buffer).unwrap();

  std::cout << req.m_method << "\n";
  std::cout << req.m_path << "\n";
  std::cout << req.m_protocol << "\n";
  std::cout << "coffee = " << req.is_coffee() << "\n";

  Response response = Response::httpcat(401);
  std::string body = response.encode();

  send(connection, body.data(), body.size(), 0);

  while (main_loop) {
    std::cout << YELLOW << "Polling for input..." << RESET << std::endl;
    event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 10);

    if (event_count == 0) break; // WTF

    std::cout << GREEN << event_count << " ready events." << RESET << std::endl;
    for (int i = 0; i < event_count; i++) {
      if (events[i].events && events[i].events == EPOLLIN) {
        std::cout << CYAN << "Reading file descriptor: " << events[i].data.fd
                  << RESET << std::endl;
        bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
        std::cout << CYAN << "bytes read: " << bytes_read << RESET << std::endl;
        read_buffer[bytes_read] = '\0';
        std::cout << CYAN << "Read: " << read_buffer << RESET << std::endl;
      } else if (events[i].events && events[i].events == EPOLLOUT) {
      }
    }
  }

  close(connection);

  if (close(epoll_fd) == -1) {
    perror("close() failed\n");
    return 1;
  }

  close(sockfd);

  return 0;
}
