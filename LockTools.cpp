#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "LockTools.h"
#include "LogManager.h"

int namedlock_init(std::string module, std::string name) {
	std::string lockfile = stdsprintf("/dev/shm/rpcsvc%u.lock.%s.%s", getppid(), module.c_str(), name.c_str());
	int fd = open(lockfile.c_str(), O_RDWR | O_CREAT, 0777);
	if (fd < 0)
		return -1;
	LOGGER->log_message(LogManager::DEBUG, stdsprintf("namedlock_init: Initialized lock %s.%s as lockid %d", module.c_str(), name.c_str(), fd));
	return fd;
}

int namedlock_destroy(int lockid) {
	LOGGER->log_message(LogManager::DEBUG, stdsprintf("namedlock_destroy: Destroy lockid %d", lockid));
	close(lockid);
	return 0;
}

int namedlock_lock(int lockid) {
	int rv;
	do {
		rv = flock(lockid, LOCK_EX);
	} while (!(rv == -1 && errno == EINTR));
	if (rv <= 0)
		return -1;

	LOGGER->log_message(LogManager::DEBUG, stdsprintf("namedlock_lock: Acquired lockid %d", lockid));
	return 0;
}

int namedlock_trylock(int lockid) {
	int rv;
	do {
		rv = flock(lockid, LOCK_EX | LOCK_NB);
	} while (!(rv == -1 && errno == EINTR));
	if (rv <= 0)
		return -1;

	LOGGER->log_message(LogManager::DEBUG, stdsprintf("namedlock_trylock: Acquired lockid %d", lockid));
	return 0;
}

int namedlock_unlock(int lockid) {
	LOGGER->log_message(LogManager::DEBUG, stdsprintf("namedlock_unlock: Releasing lockid %d", lockid));
	return flock(lockid, LOCK_UN);
}

int namedflag_set(std::string module, std::string name) {
	std::string lockfile = stdsprintf("/dev/shm/rpcsvc%u.flag.%s.%s", getppid(), module.c_str(), name.c_str());
	int fd = open(lockfile.c_str(), O_RDWR | O_CREAT, 0777);
	if (fd < 0)
		return -1;
	close(fd);
	return 0;
}

int namedflag_clear(std::string module, std::string name) {
	std::string lockfile = stdsprintf("/dev/shm/rpcsvc%u.flag.%s.%s", getppid(), module.c_str(), name.c_str());
	if (unlink(lockfile.c_str()) < 0) {
		if (errno == ENOENT)
			return 0;
		return -1;
	}
	return 0;
}

int namedflag_test(std::string module, std::string name) {
	std::string lockfile = stdsprintf("/dev/shm/rpcsvc%u.flag.%s.%s", getppid(), module.c_str(), name.c_str());
	if (access(lockfile.c_str(), F_OK) < 0) {
		if (errno == ENOENT)
			return 0;
		return -1;
	}
	return 1;
}
