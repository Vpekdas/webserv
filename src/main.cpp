#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8383);
    inet_aton("0.0.0.0", &addr.sin_addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind() failed");
        return 1;
    }

    listen(fd, 999);


    std::string s;

    while (1)
    {
        struct sockaddr addr;
        socklen_t len;
        int new_fd = accept(fd, &addr, &len);

        if (new_fd == -1 && errno == EAGAIN)
            continue;

        char buf[32];
        std::string s = "";

        std::cout << "connection accepted (fd = " << new_fd << ")\n";
        int n;
        while(1)
        {
            n = read(new_fd, buf, 32);
            s.append(buf, n);
            if (n == 0) break;
        }
        std::cout << s << "\n";
    }

    close(fd);
}
