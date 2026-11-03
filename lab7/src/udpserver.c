
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <bufsize>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int bufsize = atoi(argv[2]);

    if (port < 1 || port > 65535 || bufsize <= 0) {
        printf("Invalid port or buffer size\n");
        return 1;
    }

    int sockfd, n;
    char *mesg = malloc((size_t)bufsize + 1);
    char ipadr[16];
    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;

    if (mesg == NULL) {
        perror("malloc");
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        free(mesg);
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind problem");
        close(sockfd);
        free(mesg);
        return 1;
    }

    printf("SERVER starts...\n");
    fflush(stdout);

    while (1) {
        socklen_t len = sizeof(cliaddr);

        if ((n = recvfrom(sockfd, mesg, bufsize, 0,
                          (struct sockaddr *)&cliaddr, &len)) < 0) {
            perror("recvfrom");
            close(sockfd);
            free(mesg);
            return 1;
        }

        mesg[n] = '\0';

        printf("REQUEST %s FROM %s : %d\n", mesg,
               inet_ntop(AF_INET, &cliaddr.sin_addr, ipadr, sizeof(ipadr)),
               ntohs(cliaddr.sin_port));

        if (sendto(sockfd, mesg, n, 0,
                   (struct sockaddr *)&cliaddr, len) < 0) {
            perror("sendto");
            close(sockfd);
            free(mesg);
            return 1;
        }
    }
}
