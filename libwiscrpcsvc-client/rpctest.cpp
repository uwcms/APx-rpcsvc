#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiscrpcsvc.h>
using namespace wisc;

#undef MEMORY_TEST

int main(int argc, char *argv[]) {
	RPCSvc rpc;
	try {
		rpc.connect(argv[1]);
	}
	catch (RPCSvc::ConnectionFailedException &e) {
		printf("Caught RPCErrorException: %s\n", e.message.c_str());
		return 1;
	}
	catch (RPCSvc::RPCException &e) {
		printf("Caught exception: %s\n", e.message.c_str());
		return 1;
	}

#define STANDARD_CATCH                                                   \
	catch (RPCSvc::NotConnectedException & e) {                          \
		printf("Caught NotConnectedException: %s\n", e.message.c_str()); \
		return 1;                                                        \
	}                                                                    \
	catch (RPCSvc::RPCErrorException & e) {                              \
		printf("Caught RPCErrorException: %s\n", e.message.c_str());     \
		return 1;                                                        \
	}                                                                    \
	catch (RPCSvc::RPCException & e) {                                   \
		printf("Caught exception: %s\n", e.message.c_str());             \
		return 1;                                                        \
	}

#define ASSERT(x)                                                      \
	do {                                                               \
		if (!(x)) {                                                    \
			printf("Assertion Failed on line %u: %s\n", __LINE__, #x); \
			return 1;                                                  \
		}                                                              \
	} while (0)

	try {
		ASSERT(!rpc.load_module("rpctest", "rpctest v0.0.0"));
		ASSERT(rpc.load_module("rpctest", "rpctest v1.0.1"));
		ASSERT(!rpc.load_module("rpctest", "rpctest v0.0.0"));
		ASSERT(rpc.load_module("rpctest", "rpctest v1.0.1"));
	}
	STANDARD_CATCH;

	RPCMsg req, rsp;

#ifdef MEMORY_TEST
	try {
		ASSERT(rpc.load_module("memory", "memory v1.0.1"));
	}
	STANDARD_CATCH;

	req = RPCMsg("memory.read");
	req.set_word("address", 0x60000000);
	req.set_word("count", 1);
	try {
		rsp = rpc.call_method(req);
	}
	STANDARD_CATCH;

	uint32_t result;
	ASSERT(rsp.get_word_array_size("data") == 1);
	rsp.get_word_array("data", &result);

	uint32_t writedata = result + 1;
	req = RPCMsg("memory.write");
	req.set_word("address", 0x60000000);
	req.set_word_array("data", &writedata, 1);
	rsp = RPCMsg();
	try {
		rsp = rpc.call_method(req);
	}
	STANDARD_CATCH;

	printf("Memory at 0x60000000: 0x%08x\n", result);
#endif

	req = RPCMsg("rpctest.rpcmsg_feature");
	req.set_word("testword", 0x12345678); // expect testword+1
	uint32_t wordarray[4] = {0xc0ffee01, 0xfeedf00d, 0xdeadbeef, 0xecadecad};
	req.set_word_array("testwordarray", wordarray, sizeof(wordarray) / sizeof(uint32_t)); // expect word[i]++

	req.set_string("teststring", "Seraphim"); // expect "Seraphim? Hello there!"
	std::vector<std::string> stringarray;
	stringarray.push_back("Piro");
	stringarray.push_back("Largo");
	stringarray.push_back("Kimiko");
	stringarray.push_back("Erika");
	req.set_string_array("teststringarray", stringarray); // expect name+"? Hello there!"

	req.set_binarydata("testblob", "Cool Thing", sizeof("Cool Thing")); // expect "Cool Thing"

	try {
		rsp = rpc.call_method(req);

		ASSERT(rsp.get_key_exists("testword"));
		ASSERT(!rsp.get_key_exists("nonexistent"));

		ASSERT(rsp.get_word("testword") == 0x12345678 + 1);
		ASSERT(rsp.get_word_array_size("testwordarray") == 4);
		uint32_t wordarray_out[4];
		rsp.get_word_array("testwordarray", wordarray_out);
		for (int i = 0; i < 4; i++)
			ASSERT(wordarray_out[i] == wordarray[i] + i);

		ASSERT(rsp.get_string_array_size("teststringarray") == stringarray.size());
		std::vector<std::string> stringarray_out = rsp.get_string_array("teststringarray");
		for (int i = 0; i < stringarray_out.size(); i++)
			ASSERT(stringarray_out[i] == stringarray[i] + "? Hello there!");

		uint32_t binarydata_out_len = rsp.get_binarydata_size("testblob");
		char binarydata_out[binarydata_out_len];
		rsp.get_binarydata("testblob", binarydata_out, binarydata_out_len);
		ASSERT(strncmp(binarydata_out, "Cool Thing", binarydata_out_len) == 0);
	}
	STANDARD_CATCH;

	printf("All basic tests passed.\n");

#ifdef MULTICONNECT_TEST
	RPCSvc rpcset[50];
	try {
		for (int i = 0; i < 40; i++) {
			printf("Connecting %d...", i);
			fflush(stdout);
			rpcset[i].connect(argv[1]);
			printf(" done.\n");
			fflush(stdout);
		}
		for (int i = 0; i < 40; i++) {
			printf("Disconnecting %d...", i);
			fflush(stdout);
			rpcset[i].disconnect();
			printf(" done.\n");
			fflush(stdout);
		}
		for (int i = 0; i < 50; i++) {
			printf("Connecting %d...", i);
			fflush(stdout);
			rpcset[i].connect(argv[1]);
			printf(" done.\n");
			fflush(stdout);
		}
	}
	catch (RPCSvc::ConnectionFailedException &e) {
		printf("Caught RPCErrorException: %s\n", e.message.c_str());
		return 1;
	}
	catch (RPCSvc::RPCException &e) {
		printf("Caught exception: %s\n", e.message.c_str());
		return 1;
	}
#endif

	return 0;
}
