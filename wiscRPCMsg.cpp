#include "wiscRPCMsg.h"
#include "proto_cpp/rpcmsg.pb.h"
#include <string.h>
#include <arpa/inet.h>

#undef	RPCMSG_DEBUG
#ifdef	RPCMSG_DEBUG
#include <stdio.h>
#define dprintf(...) do { printf(__VA_ARGS__); fflush(stdout); } while(0)
#else
#define dprintf(...)
#endif

using namespace wisc;

const char RPCMsg::key_characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._";

RPCMsg::RPCMsg()
{
	this->buf = new RPCMsgProto::RPCMsg();
}

RPCMsg::RPCMsg(std::string method_name)
{
	this->buf = new RPCMsgProto::RPCMsg();
	this->set_method(method_name);
}

RPCMsg::RPCMsg(void *export_data, uint32_t datalen)
{
	this->buf = new RPCMsgProto::RPCMsg();
	this->buf->ParseFromArray(export_data, datalen);
}

RPCMsg::RPCMsg(const RPCMsg &msg)
{
	this->buf = new RPCMsgProto::RPCMsg();
	this->buf->CopyFrom(*msg.buf);
	//this->buf->ParseFromString(msg.buf->SerializeAsString());
}

RPCMsg& RPCMsg::operator=(const RPCMsg &other)
{
	delete this->buf;
	this->buf = new RPCMsgProto::RPCMsg();
	this->buf->CopyFrom(*other.buf);
	//this->buf->ParseFromString(other.buf->SerializeAsString());
	return *this;
}

RPCMsg::~RPCMsg()
{
	delete this->buf;
}

std::string RPCMsg::serialize() const
{
	return this->buf->SerializeAsString();
}

std::string RPCMsg::get_method() const
{
	//if (!this->buf->has_method())
	//	throw BadKeyException("__method__");
	return this->buf->method();
}

RPCMsg& RPCMsg::set_method(std::string value)
{
	this->buf->set_method(value);
	return *this;
}

bool RPCMsg::get_key_exists(std::string key) const
{
	for (int i = 0; i < this->buf->string_fields_size(); ++i)
		if (key == this->buf->string_fields(i).name())
			return true;

	for (int i = 0; i < this->buf->word_fields_size(); ++i)
		if (key == this->buf->word_fields(i).name())
			return true;

	for (int i = 0; i < this->buf->stringarray_fields_size(); ++i)
		if (key == this->buf->stringarray_fields(i).name())
			return true;

	for (int i = 0; i < this->buf->wordarray_fields_size(); ++i)
		if (key == this->buf->wordarray_fields(i).name())
			return true;

	return false;
}

std::string RPCMsg::get_string(std::string key) const
{
	for (int i = 0; i < this->buf->string_fields_size(); ++i)
		if (key == this->buf->string_fields(i).name())
			return this->buf->string_fields(i).value();
	throw BadKeyException(key);
}

RPCMsg& RPCMsg::set_string(std::string key, std::string value)
{
	for (int i = 0; i < this->buf->string_fields_size(); ++i) {
		if (key == this->buf->string_fields(i).name()) {
			this->buf->mutable_string_fields(i)->set_value(value);
			return *this;
		}
	}
	RPCMsgProto::RPCString *field = this->buf->add_string_fields();
	field->set_name(key);
	field->set_value(value);
	return *this;
}

uint32_t RPCMsg::get_string_array_size(std::string key) const
{
	for (int i = 0; i < this->buf->stringarray_fields_size(); ++i)
		if (key == this->buf->stringarray_fields(i).name())
			return this->buf->stringarray_fields(i).value_size();
	throw BadKeyException(key);
}

std::vector<std::string> RPCMsg::get_string_array(std::string key) const
{
	for (int i = 0; i < this->buf->stringarray_fields_size(); ++i) {
		const RPCMsgProto::RPCStringArray &arr = this->buf->stringarray_fields(i);
		if (key == arr.name()) {
			std::vector<std::string> out;
			for (int i = 0; i < arr.value_size(); ++i)
				out.push_back(arr.value(i));
			return out;
		}
	}
	throw BadKeyException(key);
}

RPCMsg& RPCMsg::set_string_array(std::string key, std::vector<std::string> value)
{
	RPCMsgProto::RPCStringArray *arr = NULL;
	for (int i = 0; i < this->buf->stringarray_fields_size(); i++) {
		if (key == this->buf->stringarray_fields(i).name()) {
			arr = this->buf->mutable_stringarray_fields(i);
			break;
		}
	}
	if (!arr)
		arr = this->buf->add_stringarray_fields();
	arr->Clear();
	arr->set_name(key);
	for (unsigned int i = 0; i < value.size(); ++i)
		arr->add_value(value[i]);
	return *this;
}

uint32_t RPCMsg::get_word(std::string key) const
{
	for (int i = 0; i < this->buf->word_fields_size(); i++)
		if (key == this->buf->word_fields(i).name())
			return this->buf->word_fields(i).value();
	throw BadKeyException(key);
}

RPCMsg& RPCMsg::set_word(std::string key, uint32_t value)
{
	for (int i = 0; i < this->buf->word_fields_size(); ++i) {
		if (key == this->buf->word_fields(i).name()) {
			this->buf->mutable_word_fields(i)->set_value(value);
			return *this;
		}
	}
	RPCMsgProto::RPCWord *field = this->buf->add_word_fields();
	field->set_name(key);
	field->set_value(value);
	return *this;
}

uint32_t RPCMsg::get_word_array_size(std::string key) const
{
	for (int i = 0; i < this->buf->wordarray_fields_size(); ++i)
		if (key == this->buf->wordarray_fields(i).name())
			return this->buf->wordarray_fields(i).value_size();
	throw BadKeyException(key);
}

void RPCMsg::get_word_array(std::string key, uint32_t *data) const
{
	std::vector<uint32_t> arr = this->get_word_array(key);
	for (unsigned int i = 0; i < arr.size(); ++i)
		data[i] = arr[i];
}

RPCMsg& RPCMsg::set_word_array(std::string key, uint32_t *data, int count)
{
	return this->set_word_array(key, std::vector<uint32_t>(data, data+count));
}

std::vector<uint32_t> RPCMsg::get_word_array(std::string key) const
{
	for (int i = 0; i < this->buf->wordarray_fields_size(); ++i) {
		const RPCMsgProto::RPCWordArray &arr = this->buf->wordarray_fields(i);
		if (key == arr.name()) {
			std::vector<uint32_t> out;
			for (int i = 0; i < arr.value_size(); ++i)
				out.push_back(arr.value(i));
			return out;
		}
	}
	throw BadKeyException(key);
}

RPCMsg& RPCMsg::set_word_array(std::string key, const std::vector<uint32_t> &data)
{
	RPCMsgProto::RPCWordArray *arr = NULL;
	for (int i = 0; i < this->buf->wordarray_fields_size(); i++) {
		if (key == this->buf->wordarray_fields(i).name()) {
			arr = this->buf->mutable_wordarray_fields(i);
			break;
		}
	}
	if (!arr)
		arr = this->buf->add_wordarray_fields();
	arr->Clear();
	arr->set_name(key);
	for (unsigned int i = 0; i < data.size(); ++i)
		arr->add_value(data[i]);
	return *this;
}

uint32_t RPCMsg::get_binarydata_size(std::string key) const
{
	return this->get_string(key).size();
}

void RPCMsg::get_binarydata(std::string key, void *data, uint32_t bufsize) const
{
	std::string value = this->get_string(key);
	if (bufsize < value.size())
		throw BufferTooSmallException();
	value.copy(reinterpret_cast<char*>(data), value.size());
}

RPCMsg& RPCMsg::set_binarydata(std::string key, const void *data, uint32_t bufsize)
{
	this->set_string(key, std::string(reinterpret_cast<const char*>(data), bufsize));
	return *this;
}
