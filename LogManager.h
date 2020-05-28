#ifndef __LOGMANAGER_H
#define __LOGMANAGER_H

#include <string.h>
#include <string>
#include <deque>
#include <stdio.h>

std::string stdsprintf(const char *fmt, ...);

class LogManager {
	public:
		// stolen from RFC5424 (we are likely to convert this to output to syslog when syslog is working):
		enum LogLevel {
			EMERGENCY,  // System is unusable
			ALERT,      // Action must be taken immediately
			CRITICAL,   // Critical conditions
			ERROR,      // Error conditions
			WARNING,    // Warning conditions
			NOTICE,     // Normal but significant condition  - put your major normal notices here
			INFO,       // Informational messages            - put most of your normally insignificant data here
			DEBUG       // Debug-level messages              - put your noise here
		};
	protected:
		FILE *logfd;
		class LogContext {
			public:
				std::string service;
				int activity_color;
				int led_id;
				LogContext(std::string service, int activity_color = 0, int led_id = 0) : service(service), activity_color(activity_color), led_id(led_id) { };
		};
		std::deque<LogContext> active_service;
		enum LogLevel output_level;
		static void *shm;
		int ledstate;
	public:
		/*
		 * Module Authors:
		 *
		 * Below this point are things you will normally call.
		 * Above this point are things that the core will normally call.
		 */
		// See the descriptions of LogLevel above.  Normally you want INFO.
		void log_message(LogLevel level, std::string message);

		// This helper function will convert a linux errno to a std::string.
		static std::string stdstrerror(int en) {
			char err[512];
			return std::string(strerror_r(en, err, 512));
		};

		// This will inform the status indicator system that the current service is active.
		void indicate_activity();

		/*
		 * Module Authors:
		 *
		 * Skip over this section.  These should only be used by the core.
		 */
		// if logpathf is "syslog", syslog() calls will be used instead.
		LogManager(std::string logpathf, LogLevel output_level);
		void push_active_service(std::string service, int activity_color = -1, int led_id = -1);
		void pop_active_service(std::string service);

		class ScopeService {
			public:
				LogManager *logmgr;
				const std::string service;
				ScopeService(LogManager *logmgr, std::string service, int activity_color = -1, int led_id = -1)
					: logmgr(logmgr), service(service) {
						logmgr->push_active_service(this->service, activity_color, led_id);
					};
				~ScopeService() {
					if (logmgr)
						logmgr->pop_active_service(this->service);
				};
		};
};

/* Initialized in run_client.cpp
 *
 * The standard logger output for the system
 */
extern LogManager *LOGGER;

#endif
