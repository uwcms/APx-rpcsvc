#define PROTOVER 3
#define MIN_PROTOVER 3
#define MODULES_DIR "/mnt/persistent/rpcmodules"
#define LOG_PATH_FORMAT "syslog"
// #define LOG_PATH_FORMAT "/var/log/rpcsvc/rpcsvc.%s.%u.log"
#define LOG_OUTPUT_LEVEL LogManager::INFO

/* 8 megabyte max message.
 * The Zynq only has so much RAM, and we serve many clients.
 * Let me know if you need this larger for any reason.
 */
#define MAX_MSGLEN (1024*1024*8)

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
// <zlib.h>
extern "C" {
	extern unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len);
};
// </zlib.h>

#include "wiscRPCMsg.h"
#include "ModuleManager.h"
using namespace wisc;

// #define ERROR_DIE(...) return 1

//#define dprintf(...) printf(__VA_ARGS__)
#define dprintf(...)
#define ERROR_DIE(...) do { LOGGER->log_message(LogManager::ERROR, stdsprintf(__VA_ARGS__)); return 1; } while (0)

#if 0
ssize_t timeout_read(int fd, void *buf, size_t count, int timeout)
{
	fd_set rfd;

	struct timeval tvtimeout;
	tvtimeout.tv_sec = timeout;
	tvtimeout.tv_usec = 0;
	time_t start = time(NULL);

	while (tvtimeout.tv_sec > 0 || (tvtimeout.tv_sec == 0 && tvtimeout.tv_usec > 0)) {
		if (start+timeout < time(NULL))
			break; // failsafe

		FD_ZERO(&rfd);
		FD_SET(fd, &rfd);
		int rv = select(fd+1, &rfd, NULL, NULL, &tvtimeout);
		if (rv < 0)
			return -1;
		if (rv == 0)
			continue;

		return read(fd, buf, count);
	}
	return 0;
}
#endif

std::string get_ip4_string(int socket)
{
	struct sockaddr_in remote_address;
	socklen_t remote_address_len = sizeof(remote_address);
	if (getpeername(socket, reinterpret_cast<sockaddr*>(&remote_address), &remote_address_len) < 0)
		return "";
	char buf[INET_ADDRSTRLEN];
	memset(buf, 0, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &remote_address.sin_addr, buf, INET_ADDRSTRLEN);
	return std::string(buf);
}

int run_client(int clientfd) {
	/* RPC Protocol Communications
	 *
	 * Initial Handshake:
	 * Send: uint32_t server_protocol_version; // network byte order
	 * Recv: uint32_t client_protocol_version; // network byte order
	 * Hang up if incompatible.
	 *
	 * Runtime:
	 * Recv: uint32_t msglen_bytes; // network byte order
	 * Recv: uint32_t msg_crc32; // network byte order
	 * Recv: uint8_t serialized_rpcmsg;
	 * Send: uint32_t msglen_bytes; // network byte order
	 * Send: uint32_t msg_crc32; // network byte order
	 * Send: uint8_t serialized_rpcmsg;
	 */

	// Create our own logger for this client.  We leak this.  It will be cleaned up on close.
	LOGGER = new LogManager(LOG_PATH_FORMAT, LOG_OUTPUT_LEVEL);
	LOGGER->push_active_service("rpcsvc");

	LOGGER->log_message(LogManager::NOTICE, stdsprintf("Client connected from %s", get_ip4_string(clientfd).c_str()));

	ModuleManager modmgr;
#ifdef LOAD_ALL_MODULES
	if (modmgr.load_modules_dir(MODULES_DIR) < 0)
		ERROR_DIE("Unable to load modules");
#endif

	uint32_t protover = htonl(PROTOVER);
	if (write(clientfd, &protover, 4) != 4)
		ERROR_DIE("Unable to write protocol version to client");
	LOGGER->log_message(LogManager::DEBUG, stdsprintf("Wrote protocol version to client: %u", PROTOVER));
	if (recv(clientfd, &protover, 4, MSG_WAITALL) != 4)
		ERROR_DIE("Unable to read protocol version from client");
	LOGGER->log_message(LogManager::DEBUG, stdsprintf("Read protocol version from client: %u", ntohl(protover)));
	if (ntohl(protover) < MIN_PROTOVER)
		ERROR_DIE("Client is too old");

	while (1) {
		uint32_t reqlen = 0, reqcrc;
		int rv;
		rv = recv(clientfd, &reqlen, 4, MSG_WAITALL);
		if (rv == 0) {
			LOGGER->log_message(LogManager::NOTICE, "Client disconnected cleanly");
			return 0;
		}
		if (rv != 4)
			ERROR_DIE("Unable to read request length from client: rv = %d, buffer = 0x%08x, errno = %d", rv, reqlen, errno);
		reqlen = ntohl(reqlen);
		if (reqlen > MAX_MSGLEN)
			ERROR_DIE("Client request too long: %u bytes", reqlen);
		if (recv(clientfd, &reqcrc, 4, MSG_WAITALL) != 4)
			ERROR_DIE("Unable to read request CRC32 from client");
		reqcrc = ntohl(reqcrc);

		uint32_t bytesread = 0;
		char reqmsg[reqlen];
		while (bytesread < reqlen) {
			int rv = read(clientfd, reqmsg+bytesread, reqlen-bytesread);
			if (rv <= 0)
				ERROR_DIE("Error reading request message from client");
			bytesread += rv;
		}

		if (reqcrc != crc32(0, reinterpret_cast<const unsigned char *>(reqmsg), reqlen))
			ERROR_DIE("Invalid request CRC32 from client");

		// TODO: Make these errors pass through rpcerror rather than disconnect.
		//       (but only where appropriate.  anything corruptionlike should drop us)
		RPCMsg rpcreq;
		try {
			rpcreq = RPCMsg(reqmsg, reqlen);
		}
		catch (RPCMsg::CorruptMessageException &e) {
			ERROR_DIE("Corrupt RPC Message received.  Diagnostic: %s", e.reason.c_str());
		}
		RPCMsg rpcrsp(rpcreq.get_method());

		try {
			if (rpcreq.get_method() == "module.load") {
				// This special case will be handled here.

				if (!modmgr.load_module(MODULES_DIR, rpcreq.get_string("module"), rpcreq.get_string("module_version_key")))
					rpcrsp.set_string("rpcerror", "Unable to load the requested module.");
			}
			else
				modmgr.invoke_method(rpcreq.get_method(), &rpcreq, &rpcrsp);
		}
		catch (RPCMsg::TypeException &e) {
			rpcrsp.set_string("rpcerror", std::string("RPCMsg::TypeException Occurred"));
		}
		catch (RPCMsg::BadKeyException &e) {
			rpcrsp.set_string("rpcerror", std::string("RPCMsg::BadKeyException Occurred"));
		}

		std::string rsp = rpcrsp.serialize();
		uint32_t rsplen = rsp.size();

#if 0
		// -------- DEBUG BEGIN --------
		dprintf("Debug Message Seralize:  [rsplen=%u]\n", rsplen);
		for (uint32_t i = 0; i < rsplen; i++) {
			if (rsp.c_str()[i] >= 32 && rsp.c_str()[i] <= 127)
				dprintf("%3c", rsp.c_str()[i]);
			else
				dprintf("   ");
		}
		dprintf("\n");
		for (uint32_t i = 0; i < rsplen; i++)
			dprintf(" %02x", rsp.c_str()[i]);
		dprintf("\n");
		// -------- DEBUG END --------
#endif

		uint32_t rspcrc = crc32(0, reinterpret_cast<const unsigned char *>(rsp.data()), rsplen);

		rsplen = htonl(rsplen);
		if (write(clientfd, &rsplen, 4) != 4)
			ERROR_DIE("Unable to write request length to client");
		rspcrc = htonl(rspcrc);
		if (write(clientfd, &rspcrc, 4) != 4)
			ERROR_DIE("Unable to write request CRC32 to client");
		uint32_t bytes_written = 0;
		while (bytes_written < rsp.size()) {
			int rv = write(clientfd, rsp.data()+bytes_written, rsp.size()-bytes_written);
			if (rv < 0)
				ERROR_DIE("Error writing response message to client");
			bytes_written += rv;
		}
	}
}
