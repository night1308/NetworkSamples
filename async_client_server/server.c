#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <event.h>

#define BACKLOG				65536

#define TIMEOUT_ACCEPT_SECS		5
#define TIMEOUT_READ_SECS		5
#define TIMEOUT_WRITE_SECS		5

static void
usage(const char *pname)
{
	(void)fprintf(stderr, "%s port\n", pname);
}

static void do_accepted(int fd, short event, void *arg);
static void do_read(int fd, short event, void *arg);
static void do_write(int fd, short event, void *arg);

static struct timeval timeout_accepted = {
	.tv_sec = TIMEOUT_ACCEPT_SECS,
	.tv_usec = 0,
};

static struct timeval timeout_read = {
	.tv_sec = TIMEOUT_READ_SECS,
	.tv_usec = 0,
};

static struct timeval timeout_write = {
	.tv_sec = TIMEOUT_WRITE_SECS,
	.tv_usec = 0,
};

#define MAX_BUF_SIZE			1024

static uint32_t buf[MAX_BUF_SIZE];
static size_t expected = 1;

static void
do_accepted(int fd, short event, void *arg)
{
	struct event *evt_accepted = arg;

	if (event == EV_TIMEOUT) {
		fprintf(stderr, "Timeout accept\n");
		event_add(evt_accepted, &timeout_accepted);
		return;
	}

	struct sockaddr remote_addr;
	socklen_t remote_addr_len = sizeof(remote_addr);
	int clientfd;

	while ((clientfd = accept(fd, &remote_addr, &remote_addr_len)) > 0) {
		char remote_addr_str[INET_ADDRSTRLEN];
		char remote_port_str[10];

		int rc = getnameinfo(&remote_addr, remote_addr_len,
					remote_addr_str,
					sizeof(remote_addr_str),
					remote_port_str,
					sizeof(remote_port_str),
					NI_NUMERICHOST | NI_NUMERICSERV);
		if (!rc) {
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(rc));
		} else {
			fprintf(stdout, "Accepted: %s:%s\n", remote_addr_str,
					remote_port_str);
		}

		struct event *evt_write = malloc(sizeof(struct event));
		if (!evt_write) {
			fprintf(stderr, "malloc evt_write\n");
			return;
		}

		event_set(evt_write, clientfd, EV_WRITE, do_write, evt_write);
		event_add(evt_write, &timeout_write);
	}
}

static void
do_write(int fd, short event, void *arg)
{
	struct event *evt_write = arg;

	if (event == EV_TIMEOUT) {
		fprintf(stderr, "Timeout write\n");
		goto fail;
	}

	fprintf(stdout, "Write: %lu\n", expected);

	buf[expected - 1] = expected - 1;

	size_t nwritten;
	ssize_t err;
	size_t expected_bytes = expected * sizeof(uint32_t);
	for (nwritten = 0; nwritten < expected_bytes; nwritten += err) {
		err = send(fd, buf + nwritten, expected_bytes - nwritten, 0);
		if (err == 0) {
			fprintf(stderr, "Remote shut down\n");
			goto fail;
		} else if (err < 0) {
			perror("send");
			goto fail;
		}
	}

	expected = ((expected + 1) % MAX_BUF_SIZE);
	if (expected == 0) {
		expected = 1;
	}

	struct event *evt_read = malloc(sizeof(struct event));
	if (!evt_read) {
		fprintf(stderr, "malloc evt_read\n");
		goto fail;
	}

	event_set(evt_read, fd, EV_READ, do_read, evt_read);
	event_add(evt_read, &timeout_read);

fail:
	free(evt_write);
}

static void
do_read(int fd, short event, void *arg)
{
	struct event *evt_read = arg;

	if (event == EV_TIMEOUT) {
		fprintf(stderr, "Timeout read\n");
		goto fail;
	}

	fprintf(stdout, "Read: %lu\n", expected);

	memset(buf, 0, expected);

	size_t nread;
	ssize_t err;
	size_t expected_bytes = expected * sizeof(uint32_t);
	for (nread = 0; nread < expected_bytes; nread += err) {
		err = recv(fd, buf + nread, expected_bytes - nread, 0);
		if (err == 0) {
			fprintf(stderr, "Remote shut down\n");
			goto fail;
		} else if (err < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				event_add(evt_read, &timeout_read);
				return;
			}
			perror("recv");
			goto fail;
		}
	}

	uint32_t i;
	for (i = 0; i < expected; i++) {
		if (buf[i] != i) {
			fprintf(stderr, "Received incorrect data\n");

			uint32_t j;
			for (j = 0; j < expected; j++) {
				fprintf(stderr, "buf[%u] == %u != %u\n", j, buf[j], j);
			}
			goto fail;
		}
	}

	expected = ((expected + 1) % MAX_BUF_SIZE);
	if (expected == 0) {
		expected = 1;
	}

	struct event *evt_write = malloc(sizeof(struct event));
	if (!evt_write) {
		fprintf(stderr, "malloc evt_write\n");
		goto fail;
	}

	event_set(evt_write, fd, EV_WRITE, do_write, evt_write);
	event_add(evt_write, &timeout_write);

fail:
	free(evt_read);
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	buf[expected - 1] = expected - 1;

	event_init();

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	int rc = getaddrinfo(NULL, argv[1], &hints, &res);
	if (rc != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
		exit(EXIT_FAILURE);
	}

	if (!res) {
		fprintf(stderr, "getaddrinfo results null\n");
		exit(EXIT_FAILURE);
	}

	int sockfd = socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK, res->ai_protocol);
	if (sockfd == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	int reuse_addr = 1;
	rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
			sizeof(reuse_addr));
	if (rc == -1) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	rc = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if (rc == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);

	rc = listen(sockfd, BACKLOG);
	if (rc == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	struct event *evt_accepted = malloc(sizeof(struct event));
	if (!evt_accepted) {
		fprintf(stderr, "malloc\n");
		exit(EXIT_FAILURE);
	}

	event_set(evt_accepted, sockfd, EV_READ, do_accepted, evt_accepted);
	event_add(evt_accepted, &timeout_accepted);

	event_dispatch();

	close(sockfd);

	return EXIT_SUCCESS;
}
