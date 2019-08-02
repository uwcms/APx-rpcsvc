ifdef PETA_ROOTFS
#PETA_ROOTFS=/afs/hep.wisc.edu/home/uwhepfpga/petalinux-v2013.10-final/CTP7/build/linux/rootfs

CXXFLAGS= -fomit-frame-pointer -pipe -fno-common -fno-builtin \
	-Wall \
	-march=armv7-a -mfpu=neon -mfloat-abi=softfp \
	-mthumb-interwork -mtune=cortex-a9 \
	-DEMBED -Dlinux -D__linux__ -Dunix -fPIC \
	-I$(PETA_ROOTFS)/stage/usr/include \
	-I$(PETA_ROOTFS)/stage/include \
	-I$(PETA_ROOTFS)/libs/libmemsvc

LDLIBS= -L$(PETA_ROOTFS)/targetroot/lib \
	-L$(PETA_ROOTFS)/stage/lib \
	-L$(PETA_ROOTFS)/targetroot/usr/lib \
	-L$(PETA_ROOTFS)/stage/usr/lib

CXX=arm-xilinx-linux-gnueabi-g++
endif

APP = rpcsvc

# Add any other object files to this list below
APP_OBJS = rpcsvc.o run_client.o ModuleManager.o wiscRPCMsg.o LogManager.o LockTools.o proto_cpp/rpcmsg.pb.o

ifndef PETALINUX
all: build modules packages
else
all: build modules packages install
endif

build: $(APP)

$(APP): $(APP_OBJS)
	$(CXX) $(LDFLAGS) -o $@ -rdynamic $(APP_OBJS) $(LDLIBS) -lmemsvc -ldl -l:libz.so.1 -lrt $(patsubst -lz,-l:libz.so.1,$(shell pkg-config --libs protobuf-lite))

clean:
	chmod -fR u+w *.elf *.gdb *.o *.so modules/*.so packages/ proto_cpp/ || true
	-rm -rf $(APP) *.elf *.gdb *.o *.so modules/*.so packages/ proto_cpp/
	for I in *.h; do rm -f modules/$$I; done

ifdef PETALINUX
install: $(APP) modules packages
	rm -rf instroot *.rpm
	mkdir instroot
	TARGETDIR=$$(pwd)/instroot $(TARGETINST) -d $(APP) /bin/$(APP)
	mkdir -p instroot/usr/local/rpcmodules.preinstall
	rsync -vrh modules/*.so instroot/usr/local/rpcmodules.preinstall/
	chmod -R 0755 instroot/usr/local/rpcmodules.preinstall
	TARGETDIR=$$(pwd)/instroot $(TARGETINST) -d initpersist_rpcsvc.sh /etc/initpersist.d/50rpcsvc.sh
	#
	TARGETDIR=$$(pwd)/instroot $(TARGETINST) -p 0755 -d rpcsvc.sh /etc/init.d/rpcsvc
	TARGETDIR=$$(pwd)/instroot $(TARGETINST) -d -s /etc/init.d/rpcsvc /etc/rcS.d/S70rpcsvc
	#
	TARGETDIR=$$(pwd)/instroot $(TARGETINST) -p 0644 -d packages/module_dev.tbz2 /usr/share/doc/rpcsvc/module_dev.tbz2
	TARGETDIR=$$(pwd)/instroot $(TARGETINST) -p 0644 -d packages/client_dev.tbz2 /usr/share/doc/rpcsvc/client_dev.tbz2
	$(call FPM_AUTOVER_CALL,rpcsvc,instroot)
	$(call RPM_AUTOVER_INSTALL_CALL,rpcsvc)
endif

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

wiscRPCMsg.o: wiscRPCMsg.cpp proto_cpp

proto_cpp/%.pb.o: proto_cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $(patsubst proto_cpp/%.pb.o,proto_cpp/%.pb.cc,$@)

proto_cpp: $(wildcard *.proto)
	@mkdir -p proto_cpp/
	/usr/bin/protoc --cpp_out=proto_cpp/ $^
	@touch -c proto_cpp/

modules: $(patsubst %.cpp, %.so, $(wildcard modules/*.cpp))

modules/%.h: %.h
	ln -s ../$< modules/$<

MODULE_HEADERS = $(patsubst %.h, modules/%.h, $(wildcard *.h))

modules/optical.so: modules/optical.cpp $(MODULE_HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

modules/%.so: modules/%.cpp $(MODULE_HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $< -lmemsvc

packages: packages/module_dev.tbz2 packages/client_dev.tbz2

packages/module_dev.tbz2: $(MODULE_HEADERS)
	# Initialize package build directory
	chmod -fR u+w packages/rpcsvc_module_dev || true
	rm -rf packages/rpcsvc_module_dev
	mkdir -p packages/rpcsvc_module_dev
	# Install package files
	cp -rL modules/*.h modules/*.cpp modules/Makefile modules/README.md packages/rpcsvc_module_dev/
	#-grip packages/rpcsvc_module_dev/README.md --export
	# Generate package version data
	./gen_package_version.sh packages/rpcsvc_module_dev >> packages/rpcsvc_module_dev/RPCSVC_MANIFEST
	# Mark package files readonly
	chmod a-w packages/rpcsvc_module_dev/*
	chmod +w packages/rpcsvc_module_dev/Makefile
	# Generate package
	tar -cjhf $@ --numeric-owner --owner=0 --group=0 -C packages/ rpcsvc_module_dev

packages/client_dev.tbz2: rpcmsg.proto $(wildcard libwiscrpcsvc-client/*.h) $(wildcard libwiscrpcsvc-client/*.cpp) libwiscrpcsvc-client/Makefile
	# Initialize package build directory
	chmod -fR u+w packages/rpcsvc_client_dev || true
	rm -rf packages/rpcsvc_client_dev
	mkdir -p packages/rpcsvc_client_dev
	# Install package files
	cp -rL $^ packages/rpcsvc_client_dev/
	# Generate package version data
	./gen_package_version.sh packages/rpcsvc_client_dev >> packages/rpcsvc_client_dev/RPCSVC_MANIFEST
	# Mark package files readonly
	chmod a-w packages/rpcsvc_client_dev/*
	chmod +w packages/rpcsvc_client_dev/Makefile
	# Generate package
	tar -cjhf $@ --numeric-owner --owner=0 --group=0 -C packages/ rpcsvc_client_dev

.PHONY: all build install image packages
