#include <iostream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <poll.h>

#define PORT 23333
#define LISTEN_QUEUE_SIZE 100
#define BUFFER_SIZE 256
#define client_size 524288        /*The maximum number of clients that the process can handle concurrently*/

int init_listen();

int main(int argc, char const *argv[])
{
	int listenfd, connfd;
	int n_ready;
	pollfd client[client_size];
	char buffer[BUFFER_SIZE];

	listenfd = init_listen();

	for (auto &i : client)                /*initialized to -1*/
		i.fd = -1;
	client[0].fd = listenfd;
	client[0].events = POLLRDNORM;
	int max_client = 1;

	sockaddr_in peer_addr{};
	socklen_t peer_len = sizeof(peer_addr);
	bzero(&peer_addr, sizeof(peer_addr));

	while (true)
	{
		n_ready = poll(client, max_client, -1);
		if (client[0].revents & POLLRDNORM)            /*handle new connection*/
		{
			connfd = accept(listenfd, (sockaddr *) &peer_addr, &peer_len);

			for (int i = 1; i < client_size; i++)
				if (client[i].fd < 0)
				{
					client[i].fd = connfd;
					client[i].events = POLLRDNORM;
					max_client = std::max(max_client, i + 1);
					break;
				}

			if (--n_ready <= 0)
				continue;
		}

		for (int i = 0; i < max_client; i++)
		{
			ssize_t n_read;
			int sockfd = client[i].fd;
			if (sockfd < 0)
				continue;
			if (client[i].revents & (POLLRDNORM | POLLERR))
			{
				n_read = read(sockfd, buffer, BUFFER_SIZE);
				if (n_read < 0)
				{
					if(errno == ECONNRESET)
					{
						close(sockfd);
						client[i].fd = -1;
					}else
					{
						printf("read error\n");
						return -1;
					}
				}
				if(n_read == 0)
				{
					close(sockfd);
					client[i].fd = -1;
				}
				if(n_read >0)
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