#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "moduleapi.h"
#include <libwisci2c.h>
#include <cardconfig.h>

memsvc_handle_t memsvc;

void measure_input_power(const RPCMsg *request, RPCMsg *response) {
#ifndef WISCPARAM_SERIES_CTP7
	// This is only written for the CTP7 at the moment.
	response->set_string("error", "Unsupported function. CTP7 Only");
	LOGGER->log_message(LogManager::INFO, "Unsupported function. CTP7 Only");
	return;
#endif

	for (int i = 0; i < 3; ++i) {
		char devnode[16];
		snprintf(devnode, 16, "/dev/i2c-%d", 2+i);
		int i2cfd = open(devnode, O_RDWR);
		if (i2cfd < 0) {
			response->set_string("error", stdsprintf("Unable to open %s", devnode));
			LOGGER->log_message(LogManager::INFO, stdsprintf("Unable to open %s", devnode));
			close(i2cfd);
			return;
		}

		if (i2c_write(i2cfd, 0x54, 127, reinterpret_cast<const uint8_t*>("\x01"), 1) != 1) {
			response->set_string("error", "i2c write failure");
			LOGGER->log_message(LogManager::INFO, "i2c write failure");
			close(i2cfd);
			return;
		}

		std::vector<uint32_t> power_readings;
		for (int j = 0; j < 12; ++j) {
			uint16_t power;
			if (i2c_read(i2cfd, 0x54, 206+(2*j), reinterpret_cast<uint8_t*>(&power), 2) != 2) {
				response->set_string("error", "i2c read failure");
				LOGGER->log_message(LogManager::INFO, "i2c read failure");
				close(i2cfd);
				return;
			}
			power = __builtin_bswap16(power);
			LOGGER->log_message(LogManager::INFO, stdsprintf("raw value: 0x%04x = %u / 10 = %u", power, power, power/10));
			power_readings.push_back(power/10);
		}

		response->set_word_array(stdsprintf("CXP%u", i), power_readings);
		close(i2cfd);
	}

	int i2cfd = open("/dev/i2c-1", O_RDWR);
	if (i2cfd < 0) {
		response->set_string("error", "Unable to open i2c-1");
		LOGGER->log_message(LogManager::INFO, "Unable to open i2c-1");
		close(i2cfd);
		return;
	}

	for (int i = 0; i < 3; ++i) {
		std::vector<uint32_t> power_readings;
		for (int j = 0; j < 12; ++j) {
			uint16_t power;
			if (i2c_read(i2cfd, 0x30+i, 64+(2*j), reinterpret_cast<uint8_t*>(&power), 2) != 2) {
				response->set_string("error", "i2c read failure");
				LOGGER->log_message(LogManager::INFO, "i2c read failure");
				close(i2cfd);
				return;
			}
			power = __builtin_bswap16(power);
			power_readings.push_back(power/10);
		}

		response->set_word_array(stdsprintf("MP%u", i), power_readings);
	}
	close(i2cfd);
}

extern "C" {
	const char *module_version_key = "optical v1.0.0";
	int module_activity_color = 0;
	void module_init(ModuleManager *modmgr) {
		modmgr->register_method("optical", "measure_input_power", measure_input_power);
	}
}
