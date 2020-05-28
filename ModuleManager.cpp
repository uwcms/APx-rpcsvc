#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>

#include "ModuleManager.h"

int ModuleManager::load_modules_dir(std::string dir) {
	DIR *dh = opendir(dir.c_str());
	if (!dh)
		return -1;
	errno = 0;
	LogManager::ScopeService scopesvc(LOGGER, "ModuleManager");
	while (struct dirent *ent = readdir(dh)) {
		if (std::string(".") == ent->d_name || std::string("..") == ent->d_name)
			continue;

		LOGGER->log_message(LogManager::DEBUG, stdsprintf("Loading module %s/%s", dir.c_str(), ent->d_name));

		void *mod = dlopen((dir + "/" + ent->d_name).c_str(), RTLD_NOW | RTLD_LOCAL);
		if (!mod) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to dlopen %s/%s: %s", dir.c_str(), ent->d_name, dlerror()));
			continue;
		}

		void *initfunc = dlsym(mod, "module_init");
		if (!initfunc) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to locate initialization function in %s/%s: %s", dir.c_str(), ent->d_name, dlerror()));
			continue;
		}

		void *module_activity_color = dlsym(mod, "module_activity_color");
		if (module_activity_color)
			this->modload_activity_color = *static_cast<int *>(module_activity_color);

		void *module_led_id = dlsym(mod, "module_led_id");
		if (module_led_id)
			this->modload_led_id = *static_cast<int *>(module_led_id);

		LOGGER->log_message(LogManager::DEBUG, stdsprintf("Initializing loaded module %s/%s", dir.c_str(), ent->d_name));
		LOGGER->push_active_service(ent->d_name, this->modload_activity_color, this->modload_led_id);
		reinterpret_cast<void (*)(ModuleManager *)>(initfunc)(this);
		LOGGER->pop_active_service(ent->d_name);
		this->modulestate[dir + "/" + ent->d_name] = 1;
		LOGGER->log_message(LogManager::DEBUG, stdsprintf("Completed module initialization for %s/%s", dir.c_str(), ent->d_name));

		this->modload_activity_color = 0;
		this->modload_led_id = 0;
		errno = 0; // for next readdir();
	}
	if (errno) {
		closedir(dh);
		return -1;
	}
	closedir(dh);
	return 0;
}

bool ModuleManager::load_module(std::string dir, std::string mod_name, std::string version_key) {
	LogManager::ScopeService scopesvc(LOGGER, "ModuleManager");

	if (mod_name.find("/") != std::string::npos) {
		LOGGER->log_message(LogManager::CRITICAL, stdsprintf("Security violation.  Module name \"%s\" contains a '/'!", mod_name.c_str()));
		return false;
	}

	LOGGER->log_message(LogManager::DEBUG, stdsprintf("Loading module %s/%s.so", dir.c_str(), mod_name.c_str()));
	void *mod = dlopen((dir + "/" + mod_name + ".so").c_str(), RTLD_NOW | RTLD_LOCAL);
	if (!mod) {
		LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to dlopen %s/%s.so: %s", dir.c_str(), mod_name.c_str(), dlerror()));
		return false;
	}

	void *module_version_key_sym = dlsym(mod, "module_version_key");
	if (!module_version_key_sym) {
		LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to locate version key in %s/%s.so: %s", dir.c_str(), mod_name.c_str(), dlerror()));
		return false;
	}

	const char *module_version_key = reinterpret_cast<const char *>(*reinterpret_cast<void **>(module_version_key_sym));
	if (version_key != module_version_key) {
		LOGGER->log_message(LogManager::WARNING, stdsprintf("Bad version key for %s/%s.so: \"%s\"!=\"%s\"", dir.c_str(), mod_name.c_str(), version_key.c_str(), module_version_key));
		return false;
	}

	if (this->modulestate[dir + "/" + mod_name + ".so"] == 1) {
		LOGGER->log_message(LogManager::INFO, stdsprintf("Module %s/%s.so is already loaded", dir.c_str(), mod_name.c_str()));
		return true;
	}

	void *initfunc = dlsym(mod, "module_init");
	if (!initfunc) {
		LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to locate initialization function in %s/%s.so: %s", dir.c_str(), mod_name.c_str(), dlerror()));
		return false;
	}

	void *module_activity_color = dlsym(mod, "module_activity_color");
	if (module_activity_color)
		this->modload_activity_color = *static_cast<int *>(module_activity_color);

	void *module_led_id = dlsym(mod, "module_led_id");
	if (module_led_id)
		this->modload_led_id = *static_cast<int *>(module_led_id);

	LOGGER->log_message(LogManager::DEBUG, stdsprintf("Initializing loaded module %s/%s.so", dir.c_str(), mod_name.c_str()));
	LOGGER->push_active_service(mod_name, this->modload_activity_color, this->modload_led_id);
	reinterpret_cast<void (*)(ModuleManager *)>(initfunc)(this);
	LOGGER->pop_active_service(mod_name);
	this->modulestate[dir + "/" + mod_name + ".so"] = 1;
	LOGGER->log_message(LogManager::DEBUG, stdsprintf("Completed module initialization for %s/%s.so", dir.c_str(), mod_name.c_str()));

	this->modload_activity_color = 0;
	this->modload_led_id = 0;
	return true;
}

void ModuleManager::register_method(std::string service, std::string method, rpc_method_t func) {
	this->methods.insert(
	    std::pair<std::string, ModuleMethod>(service + std::string(".") + method, ModuleMethod(func, this->modload_activity_color, this->modload_led_id)));
	LOGGER->log_message(LogManager::DEBUG, stdsprintf("Registered method %s.%s", service.c_str(), method.c_str()));
}

void ModuleManager::invoke_method(std::string method, RPCMsg *request, RPCMsg *response) {
	std::map<std::string, ModuleMethod>::iterator mmi = this->methods.find(method);
	if (mmi == this->methods.end()) {
		LOGGER->log_message(LogManager::ERROR, std::string("Unknown method called: ") + method);
		response->set_string("rpcerror", "Unknown method");
		return;
	}
	LogManager::ScopeService scopesvc(LOGGER, method, mmi->second.activity_color);
	LOGGER->indicate_activity();
	mmi->second.method(request, response);
}
