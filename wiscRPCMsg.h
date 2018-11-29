#ifndef __RPCMSG_H
#define __RPCMSG_H

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

/*
 * This is a generic RPC Message, it can contain whatever parameters are
 * necessary for the request.  The parameter "method" determines the function
 * to be called on the remote system.  i.e. "memory.read"
 */

namespace wisc {
	namespace RPCMsgProto {
		class RPCMsg;
	};
	class RPCMsg {
		protected:
			RPCMsgProto::RPCMsg *buf;

		public:
			static const char key_characters[];
			// "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._";

			class BadKeyException {
				public:
					const std::string key;
					BadKeyException(std::string key) : key(key) { abort(); };
			};
			class TypeException { };
			class BufferTooSmallException { };
			class CorruptMessageException {
				public:
					const std::string reason;
					CorruptMessageException(std::string reason) : reason(reason) { };
			};

			RPCMsg();
			RPCMsg(std::string method_name);
			RPCMsg(void *serial_data, uint32_t datalen);
			RPCMsg(const RPCMsg &msg);
			RPCMsg& operator=(const RPCMsg &other);
			~RPCMsg();
			std::string serialize() const;

			std::string get_method() const;
			RPCMsg& set_method(std::string value);

			bool get_key_exists(std::string key) const;

			std::string get_string(std::string key) const;
			RPCMsg& set_string(std::string key, std::string value);

			uint32_t get_string_array_size(std::string key) const;
			std::vector<std::string> get_string_array(std::string key) const;
			RPCMsg& set_string_array(std::string key, std::vector<std::string> value);

			uint32_t get_word(std::string key) const;
			RPCMsg& set_word(std::string key, uint32_t value);

			uint32_t get_word_array_size(std::string key) const;
			void get_word_array(std::string key, uint32_t *data) const;
			RPCMsg& set_word_array(std::string key, uint32_t *data, int count);
			std::vector<uint32_t> get_word_array(std::string key) const;
			RPCMsg& set_word_array(std::string key, const std::vector<uint32_t> &data);

			uint32_t get_binarydata_size(std::string key) const;
			void get_binarydata(std::string key, void *data, uint32_t bufsize) const;
			RPCMsg& set_binarydata(std::string key, const void *data, uint32_t bufsize);
	};
};

#endif
