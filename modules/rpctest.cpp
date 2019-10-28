#include <rpcsvc/moduleapi.h>
#include <string>
#include <vector>

void rpcmsg_feature(const RPCMsg *request, RPCMsg *response) {
	if (request->get_key_exists("nonexistent")) {
		response->set_string("rpcerror", "Nonexistent key found");
		return;
	}
	if (!request->get_key_exists("testword")) {
		response->set_string("rpcerror", "testword key not found");
		return;
	}

	uint32_t testword = request->get_word("testword");
	response->set_word("testword", testword+1);

	uint32_t arraysize = request->get_word_array_size("testwordarray");
	uint32_t data[arraysize];
	request->get_word_array("testwordarray", data);
	for (uint32_t i = 0; i < arraysize; i++)
		data[i]+=i;
	response->set_word_array("testwordarray", data, arraysize);


	std::string teststring = request->get_string("teststring");
	response->set_string("teststring", teststring+"? Hello there!");

	arraysize = request->get_string_array_size("teststringarray");
	std::vector<std::string> stringarray = request->get_string_array("teststringarray");
	for (std::vector<std::string>::iterator it = stringarray.begin(); it != stringarray.end(); it++)
		it->append("? Hello there!");
	response->set_string_array("teststringarray", stringarray);

	arraysize = request->get_binarydata_size("testblob");
	uint8_t blobdata[arraysize];
	request->get_binarydata("testblob", blobdata, arraysize);
	response->set_binarydata("testblob", blobdata, arraysize);
}

extern "C" {
	const char *module_version_key = "rpctest v1.0.1";
	int module_activity_color = 2;
	void module_init(ModuleManager *modmgr) {
		modmgr->register_method("rpctest", "rpcmsg_feature", rpcmsg_feature);
	}
}
