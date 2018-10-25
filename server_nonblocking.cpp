#include <iostream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cstring>
#include <cstdio>

#define PORT 23333
#define LISTEN_QUEUE_SIZE 100
#define BUFFER_SIZE 256
#define client_size 524288        /*The maximum number of clients that the process can handle concurrently*/

int init_listen();

bool set_nonblocking(int);

int main(int argc, char const *argv[])
{
	int listenfd, connfd;
	char buffer[BUFFER_SIZE];
	int clients[client_size];
	int max_client = -1;

	listenfd = init_listen();

	if (!set_nonblocking(listenfd))
	{
		printf("listening socket set nonblocking failed");
		return -1;
	}

	for (int &i:clients)
		i = -1;

	sockaddr_in peer_addr{};
	socklen_t peer_len = sizeof(peer_addr);
	bzero(&peer_addr, sizeof(peer_addr));


	while (true)
	{
		connfd = accept(listenfd, (sockaddr *) &peer_addr, &peer_len);
		if (connfd == -1 && errno == EWOULDBLOCK)
		{
			errno = 0;
		}
		else
		{
			if (!set_nonblocking(connfd))
			{
				printf("connecting socket set nonblocking failed");
				return -1;
			}
			for (int i = 0; i <= max_client + 1; i++)
			{
				if (clients[i] == -1)
				{
					clients[i] = connfd;
					max_client = std::max(max_client, i);
					break;
				}
			}
		}
		for (int i = 0; i <= max_client; i++)
		{
			ssize_t n_read;
			n_read = read(clients[i], buffer, BUFFER_SIZE);
			if (n_read <= 0)
			{
				if (errno == EWOULDBLOCK)
				{
					errno = 0;
					continue;
				} else
				{
					close(clients[i]);
					clients[i] = -1;
				}
			} else
			{
				write(clients[i], buffer, n_read);
			}
		}
	}
}

int init_listen()
{
	int listenfd;
	sockaddr_in saddr{};
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	saddr.sin_addr.s_addr = INADDR_ANY;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("create socket error!\n");
		abort();
	}
	if (bind(listenfd, (sockaddr *) &saddr, sizeof(saddr)) == -1)
	{
		printf("bind socket error!, errno=%s\n", strerror(errno));
		abort();
	}

	if (listen(listenfd, LISTEN_QUEUE_SIZE) == -1)
	{
		printf("listen error!\n");
		abort();
	}
	return listenfd;
}

bool set_nonblocking(int fd)
{
	int val;
	if ((val = fcntl(fd, F_GETFL, 0)) < 0)
		return false;
	if (fcntl(fd, F_SETFL, val | O_NONBLOCK) < 0)
		return false;
	return true;
}