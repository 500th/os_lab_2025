
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

    int sockfd, n;
    char *sendline = malloc(bufsize);
    char *recvline = malloc((size_t)bufsize + 1);
    struct sockaddr_in servaddr;

    if (sendline == NULL || recvline == NULL) {
        perror("malloc");
        free(sendline);
        free(recvline);
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address\n");
        free(sendline);
        free(recvline);
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        free(sendline);
        free(recvline);
        return 1;
    }

    printf("Enter string\n");
    fflush(stdout);

    while ((n = read(0, sendline, bufsize)) > 0) {
        if (sendto(sockfd, sendline, n, 0,
                   (struct sockaddr *)&servaddr,
                   sizeof(servaddr)) < 0) {
            perror("sendto problem");
            close(sockfd);
            free(sendline);
            free(recvline);
            return 1;
        }

        int received = recvfrom(sockfd, recvline, bufsize, 0,
                                NULL, NULL);

        if (received == -1) {
            perror("recvfrom problem");
            close(sockfd);
            free(sendline);
            free(recvline);
            return 1;
        }

        recvline[received] = '\0';

        printf("REPLY FROM SERVER= %s", recvline);
    }

    close(sockfd);
    free(sendline);
    free(recvline);
    return 0;
}
