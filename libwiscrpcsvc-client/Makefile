PROTOBUF_CHECK=$(shell ld -o/dev/null -lprotobuf-lite 2>&1 | grep -c 'ld: cannot find -lprotobuf-lite')
ifneq ($(PROTOBUF_CHECK),0)
$(error Please install the package protobuf-lite-devel)
endif

build: lib test

test: rpctest

lib: libwiscrpcsvc.so

rpctest: rpctest.cpp libwiscrpcsvc.so
	g++ -o rpctest  rpctest.cpp -I. -L. -lwiscrpcsvc

libwiscrpcsvc.so: libwiscrpcsvc.o wiscRPCMsg.o proto_cpp/rpcmsg.pb.o
	g++ -shared -fPIC -o libwiscrpcsvc.so $^ -lz $(shell pkg-config --libs protobuf-lite)

%.o: %.cpp wiscrpcsvc.h wiscRPCMsg.h proto_cpp
	g++ -c -fPIC -o $@ $<

proto_cpp/%.pb.o: proto_cpp
	g++ -c -fPIC -o $@ $(patsubst proto_cpp/%.pb.o,proto_cpp/%.pb.cc,$@)

proto_cpp: $(wildcard *.proto)
	@mkdir -p proto_cpp/
	protoc --cpp_out=proto_cpp/ $^
	@touch -c proto_cpp/

clean:
	-rm -rf *.elf *.gdb *.o *.so rpctest proto_cpp/

.PHONY: build clean lib test
