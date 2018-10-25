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
int init_listen();

int main(int argc, char const *argv[])
{
	int listenfd, connfd;
	
	listenfd = init_listen();

	sockaddr_in peer_addr{};
	socklen_t peer_len = sizeof(peer_addr);
	bzero(&peer_addr, sizeof(peer_addr));

	while (true)
	{
		if ((connfd = accept(listenfd, (sockaddr *) &peer_addr, &peer_len)) == -1)
		{
			printf("accept error!,errno=%s\n", strerror(errno));
			return -1;
		}
        
		if (fork() == 0)
		{
			close(listenfd);
			echo_serv(connfd);
			close(connfd);
			return 0;
		}
	}
}

void echo_serv(int connfd)
{
	char buffer[BUFFER_SIZE];
	bzero(buffer, sizeof(buffer));
	while (read(connfd, buffer, BUFFER_SIZE) > 0)
	{
		write(connfd, buffer, strlen(buffer));
		bzero(buffer, sizeof(buffer));
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
