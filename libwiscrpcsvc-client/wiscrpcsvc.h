#ifndef __LIBRPCSVC_H
#define __LIBRPCSVC_H

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include "wiscRPCMsg.h"

namespace wisc {
	class RPCSvc {
		protected:
			int fd;
		public:
			/* Exceptions:
			 *
			 * RPCException                - parent class for all RPCSvc exceptions
			 *   NotConnectedException     - the RPCSvc connection is not open
			 *   ConnectionFailedException - unable to connect
			 *   RPCErrorException         - a RPC error occured
			 */
			class RPCException {
				public:
					std::string message;
					RPCException(std::string message) : message(message) { };
			};
#define __LIBRPCSVC_EXCEPTION(e, p) class e : public p { public: e(std::string message) : p(message) { }; }
			__LIBRPCSVC_EXCEPTION(NotConnectedException, RPCException);
			__LIBRPCSVC_EXCEPTION(ConnectionFailedException, RPCException);
			__LIBRPCSVC_EXCEPTION(RPCErrorException, RPCException);
#undef __LIBRPCSVC_EXCEPTION

			RPCSvc() : fd(-1) { };
			void connect(std::string host, uint16_t port);
			void connect(std::string host) { this->connect(host, 9812); };
			void disconnect();
			bool load_module(std::string module, std::string module_version_key);
			RPCMsg call_method(const RPCMsg &reqmsg);
			~RPCSvc();
		private:
			RPCSvc(const RPCSvc& other) { abort(); }; // Unsupported.  Pass by reference.
			RPCSvc& operator=(const RPCSvc& other) { abort(); } // Unsupported.  Pass by reference.
	};
};

#endif
