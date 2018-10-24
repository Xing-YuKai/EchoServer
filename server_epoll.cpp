#include <iostream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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

int main(int argc, char const *argv[])
{
	int listenfd, connfd, epollfd;
	int n_ready;
	epoll_event ev;
	epoll_event evlist[client_size];
	char buffer[BUFFER_SIZE];

	epollfd = epoll_create(client_size);

	listenfd = init_listen();

	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);


	sockaddr_in peer_addr{};
	socklen_t peer_len = sizeof(peer_addr);
	bzero(&peer_addr, sizeof(peer_addr));

	while (true)
	{
		n_ready = epoll_wait(epollfd, evlist, client_size, -1);
		if(n_ready == -1 && errno == EINTR)
			continue;

		for (int i = 0; i < n_ready; i++)
		{
			if ((evlist[i].events & EPOLLIN) && (evlist[i].data.fd == listenfd))
			{
				connfd = accept(listenfd, (sockaddr *) &peer_addr, &peer_len);
				ev.events = EPOLLIN;
				ev.data.fd = connfd;
				epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
			}
			if ((evlist[i].events & EPOLLIN) && (evlist[i].data.fd != listenfd))
			{
				int sockfd = evlist[i].data.fd;
				ssize_t n_read;
				n_read = read(sockfd, buffer, BUFFER_SIZE);
				if (n_read <= 0)
				{
					epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
					close(sockfd);
				} else
				{
					write(sockfd, buffer, n_read);
				}

			}
			if (evlist[i].events & (EPOLLERR | EPOLLHUP))
			{
				int sockfd = evlist[i].data.fd;
				epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
				close(sockfd);
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