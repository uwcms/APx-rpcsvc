#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "LogManager.h"

#define MAX_CLIENTS 50

#define IP_ACL_PATH "/mnt/persistent/config/rpcsvc.acl"

#undef	NODAEMON

//#define dprintf(...) printf(__VA_ARGS__)
#define dprintf(...)

int run_client(int clientfd); // run_client.cpp

int cleanup_devshm_locks()
{
	char prefix[32];
	snprintf(prefix, 32, "rpcsvc%u.lock.", getpid());
	DIR *devshm = opendir("/dev/shm");
	if (!devshm) {
		printf("Unable to open /dev/shm for listing %d (%s)\n", errno, strerror(errno));
		return -1;
	}
	while (struct dirent *d = readdir(devshm)) {
		if (strncmp(prefix, d->d_name, 32) == 0) {
			std::string pathtokill = std::string("/dev/shm/")+d->d_name;
			unlink(pathtokill.c_str());
		}
	}
	closedir(devshm);
	return 0;
}

void do_fork() {
	if (fork())
		exit(0);
	setsid();
	if (fork())
		exit(0);
}

static inline int ip2string(uint32_t ip, char *str, int strsize) // str must be at least 16 bytes for an address
{
	return snprintf(str, strsize, "%hhu.%hhu.%hhu.%hhu",
			(ip >> 24) & 0xff,
			(ip >> 16) & 0xff,
			(ip >>  8) & 0xff,
			(ip      ) & 0xff
			);
}

static bool authorize_ip(uint32_t ip) 
{
	FILE *ipacls = fopen(IP_ACL_PATH, "r");
	if (!ipacls) {
		return false; // This is a strict allow set.  Missing file = empty set.
	}

	char linebuf[1024];

	while (fgets(linebuf, 1024, ipacls) != NULL) {
		// Ignore comments
		for (int i = 0; i < 1024; i++) {
			if (linebuf[i] == '#' || linebuf[i] == '\n') {
				linebuf[i] = '\0';
				break;
			}
		}
		// Ignore blank lines
		if (linebuf[0] == '\0')
			continue;

		uint8_t ipdata[4];
		uint8_t cidrlen;
		if (sscanf(linebuf, "%hhu.%hhu.%hhu.%hhu/%hhu", ipdata, ipdata+1, ipdata+2, ipdata+3, &cidrlen) == 5) {
			// We have a valid allow line.
			uint32_t allowip = (ipdata[0]<<24)|(ipdata[1]<<16)|(ipdata[2]<<8)|(ipdata[3]);
			uint32_t cidrmask = 0xFFFFFFFF << (32-cidrlen);

			dprintf("Allowed IP Range: %hhu.%hhu.%hhu.%hhu/%hhu.  CIDR Mask: 0x%08x - IP:0x%08x Allow:0x%08x\n", ipdata[0], ipdata[1], ipdata[2], ipdata[3], cidrlen, cidrmask, (ip&cidrmask),(allowip&cidrmask));

			// Is the current IP allowed?
			if ((ip & cidrmask) == (allowip & cidrmask)) {
				fclose(ipacls);
				return true;
			}
		}
	}
	fclose(ipacls);
	return false;
}

void handle_client_connection(int clientfd, struct sockaddr_in &client_addr, socklen_t client_addr_len, int &numchildren);

int main(int argc, char *argv[])
{
	if (cleanup_devshm_locks() < 0)
		return 1;

	int listenfd;
	struct sockaddr_in serv_addr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		printf("Unable to open socket: %d (%s)\n", errno, strerror(errno));
		return 1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(9812);

	int optval = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(int));

	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Unable to bind address: %d (%s)\n", errno, strerror(errno));
		return 1;
	}

	dprintf("Listening...\n");
	listen(listenfd,5);

#ifndef NODAEMON
	do_fork();
#endif

	int numchildren = 0;
	while (1) {
		int waitstat;
		while (waitpid(0, &waitstat, WNOHANG) > 0) {
			numchildren--;
			if (numchildren == 0)
				cleanup_devshm_locks();
		}

		struct timeval tvtimeout;
		tvtimeout.tv_sec = 0;
		tvtimeout.tv_usec = 50000;

		fd_set rfd;
		FD_ZERO(&rfd);
		FD_SET(listenfd, &rfd);
		int rv = select(listenfd+1, &rfd, NULL, NULL, &tvtimeout);
		if (rv == 0 || (rv < 0 && errno == EINTR))
			continue;
		if (rv < 0)
			return 1;

		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int clientfd = accept(listenfd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
		if (clientfd >= 0) 
			handle_client_connection(clientfd, client_addr, client_addr_len, numchildren);
	}
	close(listenfd);
	return 0; 
}

void handle_client_connection(int clientfd, struct sockaddr_in &client_addr, socklen_t client_addr_len, int &numchildren)
{
	dprintf("Handling connection #%u.\n", numchildren);
	if (!authorize_ip(ntohl(client_addr.sin_addr.s_addr))) {
		close(clientfd);
		char ipaddr[16];
		ip2string(ntohl(client_addr.sin_addr.s_addr), ipaddr, 16);
		dprintf("Dropped client #%u: Unauthorized IP: %s, not forking.\n", numchildren, ipaddr);
		return;
	}
	if (numchildren >= MAX_CLIENTS) {
		close(clientfd);
		dprintf("Dropped client #%u: max client connections reached, not forking.\n", numchildren);
		return;
	}
	dprintf("Accepted client #%u, forking...\n", numchildren);

	int optval = 3; // tcp_keepalive_probes (max unacknowledged)
	if(setsockopt(clientfd, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof(optval)) < 0) {
		dprintf("Failed to set TCP_KEEPCNT.  Dropping new client\n");
		close(clientfd);
		return;
	}

	optval = 60; // tcp_keepalive_time (idle time before connection's first probe)
	if(setsockopt(clientfd, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) < 0) {
		dprintf("Failed to set TCP_KEEPIDLE.  Dropping new client\n");
		close(clientfd);
		return;
	}

	optval = 60; // tcp_keepalive_intvl (time between probes, data or not)
	if(setsockopt(clientfd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof(optval)) < 0) {
		dprintf("Failed to set TCP_KEEPINTVL.  Dropping new client\n");
		close(clientfd);
		return;
	}

	optval = 1; // enable tcp keepalive
	if(setsockopt(clientfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
		dprintf("Failed to set SO_KEEPALIVE.  Dropping new client\n");
		close(clientfd);
		return;
	}

	int pid = fork();
	if (pid == 0) {
		run_client(clientfd); // process exiting handles fd cleanup.
		exit(0); 
	}
	else if (pid > 0)
		numchildren++;
	close(clientfd); // parent has no use for this.
}
