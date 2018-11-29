#include "moduleapi.h"
#include <libmemsvc.h>

memsvc_handle_t memsvc;

void mread(const RPCMsg *request, RPCMsg *response) {
	uint32_t count = request->get_word("count");
	uint32_t addr = request->get_word("address");
	uint32_t data[count];

	if (memsvc_read(memsvc, addr, count, data) == 0) {
		response->set_word_array("data", data, count);
	}
	else {
		response->set_string("error", memsvc_get_last_error(memsvc));
		LOGGER->log_message(LogManager::INFO, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
	}
}

void mwrite(const RPCMsg *request, RPCMsg *response) {
	uint32_t count = request->get_word_array_size("data");
	uint32_t data[count];
	request->get_word_array("data", data);
	uint32_t addr = request->get_word("address");

	if (memsvc_write(memsvc, addr, count, data) != 0) {
		response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
		LOGGER->log_message(LogManager::INFO, stdsprintf("write memsvc error: %s", memsvc_get_last_error(memsvc)));
	}
}

extern "C" {
	const char *module_version_key = "memory v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("memory", "read", mread);
		modmgr->register_method("memory", "write", mwrite);
	}
}
