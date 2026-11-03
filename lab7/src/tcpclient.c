
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <ip> <port> <bufsize>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    int bufsize = atoi(argv[3]);

    if (port < 1 || port > 65535 || bufsize <= 0) {
        printf("Invalid port or buffer size\n");
        return 1;
    }

    int fd;
    int nread;
    char *buf = malloc(bufsize);
    struct sockaddr_in servaddr;

    if (buf == NULL) {
        perror("malloc");
        return 1;
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creating");
        free(buf);
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address\n");
        close(fd);
        free(buf);
        return 1;
    }

    servaddr.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(fd);
        free(buf);
        return 1;
    }

    printf("Input message to send\n");
    fflush(stdout);

    while ((nread = read(0, buf, bufsize)) > 0) {
        if (write(fd, buf, nread) < 0) {
            perror("write");
            close(fd);
            free(buf);
            return 1;
        }
    }

    close(fd);
    free(buf);
    return 0;
}
