#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cstdio>

#define PORT 23333
#define LISTEN_QUEUE_SIZE 100
#define BUFFER_SIZE 100

void echo_serv(int connfd);

int main(int argc, char const *argv[])
{

	int listenfd, connfd;
	sockaddr_in saddr{};
	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	saddr.sin_addr.s_addr = INADDR_ANY;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("create socket error!\n");
		return -1;
	}
	if (bind(listenfd, (sockaddr *) &saddr, sizeof(saddr)) == -1)
	{
		printf("bind socket error!, errno=%s\n", strerror(errno));
		return -1;
	}

	if (listen(listenfd, LISTEN_QUEUE_SIZE) == -1)
	{
		printf("listen error!\n");
		return -1;
	}

	sockaddr_in peer_addr{};
	socklen_t peer_len = sizeof(peer_addr);
	bzero(&peer_addr, sizeof(peer_addr));

	while (true)
	{
		if ((connfd = accept(listenfd, (sockaddr *) &peer_addr, &peer_len)) == -1)
		{
			printf("accept error!,errno=%s\n", strerror(errno));
			return -1;
		} else
		{
			printf("accept connfd: %s:%d\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
		}

		echo_serv(connfd);

		close(connfd);
	}
}

void echo_serv(int connfd)
{
	char buffer[BUFFER_SIZE];
	bzero(buffer, sizeof(buffer));
	while (read(connfd, buffer, BUFFER_SIZE) > 0)
	{
		printf("read:%s\n", buffer);
		write(connfd, buffer, strlen(buffer));
		bzero(buffer, sizeof(buffer));
	}
}