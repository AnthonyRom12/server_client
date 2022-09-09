#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <pthread.h>

#define err_message(msg) \
    do {perror(msg); exit(EXIT_FAILURE);} while(0)

static int create_clientfd(char const* addr, uint16_t u16port)
{
    int fd;
    struct sockaddr_in server;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) err_message("socket err\n");

    server.sin_family = AF_INET;
    server.sin_port = htons(u16port);
    inet_pton(AF_INET, addr, &server.sin_addr);

    if (connect(fd, (struct sockaddr*)&server, sizeof(server)) < 0) perror("connect err\n");

    return fd;
}

static void* routine(void* args)
{
    int fd;
    char buf[128];
    fd = create_clientfd("127.0.0.1", 10009);
    for (; ;) {
        write(fd, "Hello", strlen("hello"));
        memset(buf, '\0', sizeof(buf));
        read(fd, buf, sizeof(buf) - 1);
        fprintf(stdout, "pthreadid:%ld %s\n", pthread_self(), buf);
        usleep(100 * 1000);
    }
}


int main(void)
{
    pthread_t pids[4];

    for (int i = 0; i < sizeof(pids) / sizeof(pthread_t); ++i) {
        pthread_create(pids + i, NULL, routine, 0);
    }

    for (int i = 0; i < sizeof(pids) / sizeof(pthread_t); ++i) {
        pthread_join(pids[i], 0);
    }

    return 0;
}
