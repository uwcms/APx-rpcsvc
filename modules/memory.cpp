#include <rpcsvc/moduleapi.h>
#include <libeasymem.h>
#include <errno.h>

static void *memrange = NULL;

void mread(const RPCMsg *request, RPCMsg *response) {
	uint32_t count = request->get_word("count");
	uint32_t addr = request->get_word("address");
	uint32_t data[count];

	if (easymem_saferead32(memrange, addr, count, data, 1) == 0) {
		response->set_word_array("data", data, count);
	}
	else {
		response->set_string("error", stdsprintf("Easymem error: errno %d", errno));
		LOGGER->log_message(LogManager::INFO, stdsprintf("read easymem error: %d", errno));
	}
}

void mwrite(const RPCMsg *request, RPCMsg *response) {
	uint32_t count = request->get_word_array_size("data");
	uint32_t data[count];
	request->get_word_array("data", data);
	uint32_t addr = request->get_word("address");

	if (easymem_safewrite32(memrange, addr, count, data, 1) != 0) {
		response->set_string("error", stdsprintf("Easymem error: errno %d", errno));
		LOGGER->log_message(LogManager::INFO, stdsprintf("write easymem error: errno %d", errno));
	}
}

extern "C" {
	const char *module_version_key = "memory v1.0.2";
	int module_activity_color = 0xff0066;
	int module_led_id = 0;
	void module_init(ModuleManager *modmgr) {
		if (easymem_map_uio(&memrange, "/dev/test_bram", 0, 0x2000, 0) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to map UIO /dev/test_bram: errno %d", errno));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on that mapping.
		}
		modmgr->register_method("memory", "read", mread);
		modmgr->register_method("memory", "write", mwrite);
	}
}
