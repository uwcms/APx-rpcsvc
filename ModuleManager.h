#ifndef __MODULE_MANAGER_H
#define __MODULE_MANAGER_H

#include <string>
#include <map>
#include "wiscRPCMsg.h"
#include "LogManager.h"
using namespace wisc;

class ModuleManager {
	public:
		typedef void (*rpc_method_t)(const RPCMsg *req, RPCMsg *rsp);
	protected:
		class ModuleMethod {
			public:
				rpc_method_t method;
				int activity_color;
				ModuleMethod(rpc_method_t method, int activity_color) : method(method), activity_color(activity_color) { };
		};
		std::map<std::string, ModuleMethod> methods;
		std::map<std::string, int> modulestate;
		int modload_activity_color;
	public:
		ModuleManager() { };
		int load_modules_dir(std::string dir);
		bool load_module(std::string dir, std::string mod_name, std::string version_key);
		void register_method(std::string service, std::string module, rpc_method_t func);
		void invoke_method(std::string method, RPCMsg *request, RPCMsg *response);
};

#endif
