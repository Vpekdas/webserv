#define MAX_EVENTS 5
#define READ_SIZE 1024

#include "../src/colors.hpp"

#include <unistd.h>   
#include <sys/epoll.h>
#include <cstdio>

#include <iostream>


int main()
{
	int event_count;
	size_t bytes_read;
	char read_buffer[READ_SIZE + 1];
	struct epoll_event event, events[MAX_EVENTS];
	int epoll_fd = epoll_create1(0);

	if (epoll_fd == -1) {
        perror("epoll_create1() failed\n");
		return 1;
	}

	event.events = EPOLLIN | EPOLLET;
	event.data.fd = 0;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event) == -1)
	{
        perror("epoll_ctl() failed\n");
		close(epoll_fd);
		return 1;
	}

	while (1) {
        std::cout << YELLOW << "Polling for input..." << RESET << std::endl;
		event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 3000);
        std::cout << GREEN << event_count << " ready events." << RESET << std::endl;
		for (int i = 0; i < event_count; i++) {
            if (events[i].events && events[i].events == EPOLLIN) {
            std::cout << CYAN << "Reading file descriptor: " << events[i].data.fd << RESET << std::endl;
			bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
            std::cout << CYAN << "bytes read: " << bytes_read << RESET << std::endl;
			read_buffer[bytes_read] = '\0';
            std::cout << CYAN << "Read: " << read_buffer << RESET << std::endl;
            }
		}
	}

	if (close(epoll_fd)) {
        perror("close() failed\n");
		return 1;
	}

	return 0;
}