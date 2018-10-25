#include <iostream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>

#define PORT 23333
#define LISTEN_QUEUE_SIZE 100
#define BUFFER_SIZE 256
#define client_size FD_SETSIZE-4        /*The maximum number of clients that the process can handle concurrently
										 *The number 4 indicates stdinfd, stdoutfd, stderrorfd, listenfd       */

int init_listen();

int main(int argc, char const *argv[])
{
	int listenfd, connfd;
	int maxfd, n_ready, max_client = -1;
	bool client_overflow = false;        /*If the process can't handle more client concurrently*/
	int client[client_size];            /*record the socket descriptor of client*/
	char buffer[BUFFER_SIZE];
	fd_set read_set;

	listenfd = init_listen();

	for (int &i : client)                /*initialized to -1*/
		i = -1;
	maxfd = listenfd;
	FD_ZERO(&read_set);
	FD_SET(listenfd, &read_set);

	sockaddr_in peer_addr{};
	socklen_t peer_len = sizeof(peer_addr);
	bzero(&peer_addr, sizeof(peer_addr));

	while (true)
	{
		fd_set tmp_set = read_set;
		n_ready = select(maxfd + 1, &tmp_set, NULL, NULL, NULL);
		if(n_ready == -1 && errno == EINTR)
			continue;
		if (FD_ISSET(listenfd, &tmp_set) && (!client_overflow))            /*handle new connection*/
		{
			connfd = accept(listenfd, (sockaddr *) &peer_addr, &peer_len);
			for (int i = 0; i < client_size; i++)
				if (client[i] < 0)
				{
					client[i] = connfd;
					max_client = std::max(max_client, i);
					break;
				}

			client_overflow = true;
			for (int i :client)
				if (i == -1)
					client_overflow = false;

			FD_SET(connfd, &read_set);
			maxfd = std::max(connfd, maxfd);
			if (--n_ready <= 0)
				continue;
		}

		for (int i = 0; i <= max_client; i++)
		{
			ssize_t n_read;
			int sockfd = client[i];
			if (sockfd < 0)
				continue;
			if (FD_ISSET(sockfd, &tmp_set))
			{
				if ((n_read = read(sockfd, buffer, BUFFER_SIZE)) <= 0)
				{
					close(sockfd);
					FD_CLR(sockfd, &read_set);
					client[i] = -1;
					client_overflow = false;
				} else
				{
					write(sockfd, buffer, n_read);
				}
				if (--n_ready <= 0)
					break;
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
