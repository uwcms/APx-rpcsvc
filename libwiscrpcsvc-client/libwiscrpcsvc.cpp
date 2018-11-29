#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

// <zlib.h>
extern "C" {
	extern unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len);
};
// </zlib.h>

#include "wiscrpcsvc.h"
using namespace wisc;

#define PROTOVER 3
#define MIN_PROTOVER 3

#undef	DEBUG_LIBRPCSVC
#ifdef	DEBUG_LIBRPCSVC
#include <stdio.h>
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

ssize_t timeout_recv(int fd, void *buf, size_t count, int timeout_seconds=30) {
	fd_set fds;
	size_t bytesread = 0;
	while (bytesread < count) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		struct timeval tvtimeout;
		tvtimeout.tv_sec = timeout_seconds;
		tvtimeout.tv_usec = 0;
		int rv = select(fd+1, &fds, NULL, NULL, &tvtimeout);
		if (rv < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (rv == 0)
			return bytesread;
		rv = recv(fd, reinterpret_cast<uint8_t*>(buf)+bytesread, count-bytesread, MSG_DONTWAIT);
		if (rv < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return bytesread;
		}
		if (rv == 0)
			return bytesread; // This is EOF, since there was no error on read.
		bytesread += rv;
	}
	return bytesread;
}

ssize_t timeout_send(int fd, const void *buf, size_t count, int timeout_seconds=30) {
	fd_set fds;
	size_t byteswritten = 0;
	while (byteswritten < count) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		struct timeval tvtimeout;
		tvtimeout.tv_sec = timeout_seconds;
		tvtimeout.tv_usec = 0;
		int rv = select(fd+1, NULL, &fds, NULL, &tvtimeout);
		if (rv < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return -1;
		}
		if (rv == 0)
			return byteswritten;
		rv = send(fd, reinterpret_cast<const uint8_t*>(buf)+byteswritten, count-byteswritten, MSG_DONTWAIT);
		if (rv < 0) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			return byteswritten;
		}
		if (rv == 0)
			return byteswritten; // This is STRANGE, since there was no error on write.
		byteswritten += rv;
	}
	return byteswritten;
}

std::string stdstrerror(int en) {
	char err[512];
	return std::string(strerror_r(en, err, 512));
}

void RPCSvc::connect(std::string host, uint16_t port) {
	struct sockaddr_in serv_addr;
	struct hostent *server;

	server = gethostbyname(host.c_str());
	if (server == NULL)
		throw ConnectionFailedException("Unable to resolve hostname");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	this->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->fd < 0)
		throw ConnectionFailedException(std::string("Unable to create socket: ")+stdstrerror(errno));

	if (::connect(this->fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  {
		int err = errno;
		close(this->fd);
		this->fd = -1;
		throw ConnectionFailedException(std::string("Unable to connect: ")+stdstrerror(err));
	}

	uint32_t protover;
	if (timeout_recv(this->fd, &protover, 4) != 4) {
		int err = errno;
		close(this->fd);
		this->fd = -1;
		if (errno)
			throw ConnectionFailedException(std::string("Unable to read protocol version from server: ")+stdstrerror(err));
		else
			throw ConnectionFailedException(std::string("Unable to read protocol version from server: Connection closed."));
	}
	dprintf("Got ver %u\n", htonl(protover));
	if (ntohl(protover) < MIN_PROTOVER) {
		close(this->fd);
		this->fd = -1;
		throw ConnectionFailedException(std::string("Server protocol version too old"));
	}

	protover = htonl(PROTOVER);
	if (timeout_send(this->fd, &protover, 4) != 4) {
		int err = errno;
		close(this->fd);
		this->fd = -1;
		throw ConnectionFailedException(std::string("Unable to write protocol version to server: ")+stdstrerror(err));
	}
	dprintf("Sent ver %u\n", PROTOVER);
}

void RPCSvc::disconnect() {
	close(this->fd);
	this->fd = -1;
}

bool RPCSvc::load_module(std::string module, std::string module_version_key)
{
	RPCMsg rpcreq("module.load");
	rpcreq.set_string("module", module);
	rpcreq.set_string("module_version_key", module_version_key);
	try {
		RPCMsg rpcrsp = this->call_method(rpcreq);
		return !rpcrsp.get_key_exists("rpcerror");
	}
	catch (RPCException &e) {
		return false;
	}
}

RPCMsg RPCSvc::call_method(const RPCMsg &reqmsg)
{
	/* RPC Protocol Communications
	 *
	 * Initial Handshake:
	 * Recv: uint32_t server_protocol_version; // network byte order
	 * Hang up if incompatible.
	 * Send: uint32_t client_protocol_version; // network byte order
	 *
	 * Runtime:
	 * Send: uint32_t msglen_bytes; // network byte order
	 * Send: uint32_t msg_crc32; // network byte order
	 * Send: uint8_t[] serialized_rpcmsg;
	 * Recv: uint32_t msglen_bytes; // network byte order
	 * Recv: uint32_t msg_crc32; // network byte order
	 * Recv: uint8_t[] serialized_rpcmsg;
	 */

	if (this->fd < 0)
		throw NotConnectedException("Not connected to rpc service");

	std::string reqstr = reqmsg.serialize();
	uint32_t reqlen = reqstr.size();
	uint32_t reqcrc = crc32(0, reinterpret_cast<const unsigned char *>(reqstr.data()), reqlen);

	reqlen = htonl(reqlen);
	if (timeout_send(this->fd, &reqlen, 4) != 4) {
		close(this->fd);
		this->fd = -1;
		throw RPCException("Unable to write request length to server");
	}
	dprintf("Wrote length %u\n", ntohl(reqlen));
	reqcrc = htonl(reqcrc);
	if (timeout_send(this->fd, &reqcrc, 4) != 4) {
		close(this->fd);
		this->fd = -1;
		throw RPCException("Unable to write request CRC32 to server");
	}
	dprintf("Wrote crc32 0x%08x\n", ntohl(reqcrc));
	uint32_t bytes_written = 0;
	while (bytes_written < reqstr.size()) {
		int rv = timeout_send(this->fd, reqstr.data()+bytes_written, reqstr.size()-bytes_written);
		if (rv < 0) {
			close(this->fd);
			this->fd = -1;
			throw RPCException("Error writing response message to server");
		}
		bytes_written += rv;
	}
	dprintf("Wrote %u request bytes\n", bytes_written);

	uint32_t rsplen, rspcrc;
	if (timeout_recv(this->fd, &rsplen, 4, 300) != 4) { // Allow a 5 minute timeout for the response execution and first return, rather than 30s for message transfer.
		close(this->fd);
		this->fd = -1;
		throw RPCException("Unable to read response length from server");
	}
	rsplen = ntohl(rsplen);
	dprintf("Read rsplen %u\n", rsplen);
	if (timeout_recv(this->fd, &rspcrc, 4) != 4) {
		close(this->fd);
		this->fd = -1;
		throw RPCException("Unable to read response CRC32 from server");
	}
	rspcrc = ntohl(rspcrc);
	dprintf("Read rspcrc 0x%08x\n", rspcrc);

	uint32_t bytesread = 0;
	char rspmsg[rsplen];
	while (bytesread < rsplen) {
		int rv = timeout_recv(this->fd, rspmsg+bytesread, rsplen-bytesread);
		if (rv < 0) {
			close(this->fd);
			this->fd = -1;
			throw RPCException("Error reading response message from server");
		}
		bytesread += rv;
	}
	dprintf("Read %u response bytes\n", bytesread);

	if (rspcrc != crc32(0, reinterpret_cast<const unsigned char *>(rspmsg), rsplen)) {
		close(this->fd);
		this->fd = -1;
		throw RPCException("Invalid request CRC32 from client");
	}

	return RPCMsg(rspmsg, rsplen);
}

RPCSvc::~RPCSvc()
{
	this->disconnect();
}
