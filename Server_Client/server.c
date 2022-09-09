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

#include <ev.h>

/* Ограничение кол-ва клиентов */
#define MAX_CLIENTS 1000

/* Ограничение длины сообщения */
#define MAX_MESSAGE_LEN (256)

#define err_message(msg) \
	do {perror(msg); exit(EXIT_FAILURE);} while (0)

/* Фиксируем кол-во клиентов */
static int client_number;

static int create_serverfd(char const *addr, uint16_t u16port)
{
	int fd;
	struct sockaddr_in server;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) err_message("socket error\n");

	server.sin_family = AF_INET;
	server.sin_port = htons(u16port);
	inet_pton(AF_INET, addr, &server.sin_addr);

	if (bind(fd, (struct sockaddr *)&server, sizeof(server)) < 0) err_message("bind error\n");

	if (listen(fd, 10) < 0) err_message("listen error\n");

	return fd;
}


static void read_cb(EV_P_ ev_io *watcher, int revents)
{
	ssize_t ret;
	char buf[MAX_MESSAGE_LEN] = {0};

	ret = recv(watcher->fd, buf, sizeof(buf) - 1, MSG_DONTWAIT);

	if (ret > 0) {
		write(watcher->fd, buf, ret);
	} else if ((ret < 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return;
	} else {
		fprintf(stdout, "client closed (fd = %d)\n", watcher->fd);

		--client_number;
		ev_io_stop(EV_A_ watcher);
		close(watcher->fd);
		free(watcher);
	}
}

static void accept_cb(EV_P_ ev_io *watcher, int revents)
{
	int connfd;
	ev_io *client;

	connfd = accept(watcher->fd, NULL, NULL);
	if (connfd > 0) {
		if (++client_number > MAX_CLIENTS) {
			close(watcher->fd);
		} else {
			client = calloc(1, sizeof(*client));
			ev_io_init(client, read_cb, connfd, EV_READ);
			ev_io_start(EV_A_ client);
		}
	}
	else if ((connfd < 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return;
	}
	else {
		close(watcher->fd);
		ev_break(EV_A_ EVBREAK_ALL);
		/* Это приведет к выходу из мэйн */
	}
}

static void start_server(char const *addr, uint16_t u16port)
{
	int fd;
#ifdef EV_MULTIPLICITY
	struct ev_loop *loop;
#else
	int loop;
#endif
	ev_io *watcher;

	fd = create_serverfd(addr, u16port);
	loop = ev_default_loop(EVFLAG_NOENV);
	watcher = calloc(1, sizeof(*watcher));
	assert(("can't alloc memory\n", loop && watcher));

	/* Устанавливаем неблокирующий флаг */
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	ev_io_init(watcher, accept_cb, fd, EV_READ);
	ev_io_start(EV_A_ watcher);
	ev_run(EV_A_ 0);

	ev_loop_destroy(EV_A);
	free(watcher);
}

static void signal_handler(int signo)
{
	switch (signo) {
		case SIGPIPE:
			break;
		default:
			// Недоступный
			break;
	}
}

int main(void)
{
	signal(SIGPIPE, signal_handler);
	start_server("127.0.0.1", 10009);

	return 0;
}








