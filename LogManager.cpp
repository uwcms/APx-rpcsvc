#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <libledmgr.h>

#include "LogManager.h"

LogManager *LOGGER = NULL;

std::string stdsprintf(const char *fmt, ...) {
	va_list va;
	va_list va2;
	va_start(va, fmt);
	va_copy(va2, va);
	size_t s = vsnprintf(NULL, 0, fmt, va);
	char str[s];
	vsnprintf(str, s + 1, fmt, va2);
	va_end(va);
	va_end(va2);
	return std::string(str);
}

LogManager::LogManager(std::string logpathf, LogLevel output_level) {
	if (logpathf == "syslog") {
		this->logfd = NULL;
		openlog("rpcsvc", LOG_PID, LOG_LOCAL0);
	}
	else {
		time_t now = time(NULL);
		struct tm *s_tm = localtime(&now);
		char timebuf[64];
		strftime(timebuf, 64, "%Y%m%dT%H:%M:%S%z", s_tm);

		std::string logfile = stdsprintf(logpathf.c_str(), timebuf, getpid());

		this->logfd = fopen(logfile.c_str(), "a");
		if (!this->logfd)
			this->logfd = stdout;
	}
	this->active_service.push_back(LogContext("LogManager"));
	this->output_level = output_level;
}

void LogManager::push_active_service(std::string service, int activity_color, int led_id) {
	if (activity_color == -1) {
		activity_color = 0;
		if (this->active_service.size())
			activity_color = this->active_service.back().activity_color;
	}
	if (led_id == -1) {
		led_id = 0;
		if (this->active_service.size())
			led_id = this->active_service.back().led_id;
	}
	this->active_service.push_back(LogContext(service, activity_color, led_id));
}

void LogManager::pop_active_service(std::string service) {
	if (this->active_service.back().service == service) {
		this->active_service.pop_back();
		if (this->active_service.size() == 0)
			this->active_service.push_back(LogContext("UNKNOWN(LogManager)", 0));
		return;
	}
	this->active_service.push_back(LogContext(stdsprintf("UNKNOWN(%s/%s)", this->active_service.back().service.c_str(), service.c_str())));
}

void LogManager::log_message(LogLevel level, std::string message) {
	if (level > this->output_level)
		return;

	if (this->logfd == NULL) {
		// Syslog Serviced
		const int int_levels[] = {LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG};
		syslog(int_levels[level], "%s: %s", this->active_service.back().service.c_str(), message.c_str());
	}
	else {
		const char *string_levels[] = {"EMERGENCY", "ALERT", "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG"};
		fprintf(this->logfd, "% 6u %-9s %s: %s\n", getpid(), string_levels[level], this->active_service.back().service.c_str(), message.c_str());
		fflush(this->logfd);
	}
}

void *LogManager::shm = NULL;

void LogManager::indicate_activity() {
	if (this->active_service.size() == 0)
		return; // No context.

	int activity_color = this->active_service.back().activity_color;
	int led_id = this->active_service.back().activity_color;

	if (activity_color)
		ledmgr_indicate(activity_color, led_id);
}
