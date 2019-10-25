CFLAGS := $(CFLAGS) -D 'RPCSVC_MODULES_DIR="$(RPCSVC_MODULES_DIR)"' -D 'RPCSVC_ACL_PATH="$(RPCSVC_ACL_PATH)"'

all: build modules packages

build: rpcsvc

rpcsvc: rpcsvc.o run_client.o ModuleManager.o wiscRPCMsg.o LogManager.o LockTools.o proto_cpp/rpcmsg.pb.o
	$(CXX) $(LDFLAGS) -o $@ -rdynamic $^ $(LDLIBS) -lmemsvc -ldl -l:libz.so.1 -lrt $(patsubst -lz,-l:libz.so.1,$(shell pkg-config --libs protobuf-lite))

clean:
	chmod -fR u+w *.elf *.gdb *.o *.so modules/*.so packages/ proto_cpp/ *.rpm || true
	-rm -rf rpcsvc *.elf *.gdb *.o *.so modules/*.so packages/ proto_cpp/ *.rpm

%.o: %.cpp
	@[ -n '$(RPCSVC_MODULES_DIR)' ] || (echo "Please define RPCSVC_MODULES_DIR."; exit 1)
	@[ -n '$(RPCSVC_ACL_PATH)' ] || (echo "Please define RPCSVC_ACL_PATH."; exit 1)
	$(CXX) -c $(CFLAGS) -o $@ $<

wiscRPCMsg.o: wiscRPCMsg.cpp proto_cpp

proto_cpp/%.pb.o: proto_cpp
	$(CXX) -c $(CFLAGS) -o $@ $(patsubst proto_cpp/%.pb.o,proto_cpp/%.pb.cc,$@)

proto_cpp: $(wildcard *.proto)
	@mkdir -p proto_cpp/
	protoc --cpp_out=proto_cpp/ $^
	@touch -c proto_cpp/

modules: $(patsubst %.cpp, %.so, $(wildcard modules/*.cpp))

modules/optical.so: modules/optical.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

modules/%.so: modules/%.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $<

packages: packages/module_dev.tbz2 packages/client_dev.tbz2

packages/module_dev.tbz2:
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

ifneq (x,x$(wildcard .git))
RPM_PKGNAME := rpcsvc
RPM_TOPDIR  := $(shell rpmbuild --eval %{_topdir})
GIT_COMMIT_DATE := $(shell date -u +%Y%m%dT%H%M%S -d "$(shell git log --format=%ci -n1 HEAD | sed -re 's/(\S+) (\S+) (\S+)/\1T\2\3/')")
RPM_VERSION := $(shell git describe --match='v[0-9]*.[0-9]*.[0-9]*' HEAD | sed -re 's/^v//;s/-([0-9]+)-(g[0-9a-f]{7})/.\1_$(GIT_COMMIT_DATE)_\2/')
RPM_LIB_VERSION := $(shell git describe --match='v[0-9]*.[0-9]*.[0-9]*' HEAD | sed -re 's/^v//;s/-([0-9]+)-(g[0-9a-f]{7})//')
RPM_GENERATED_SPEC := $(shell mktemp -t $(RPM_PKGNAME).XXXXXX.spec)
rpm:
	rm -f $(RPM_TOPDIR)/SOURCES/$(RPM_PKGNAME)-$(RPM_VERSION).tar.gz
	mkdir -p $(RPM_TOPDIR)/SOURCES
	git archive -9 --prefix $(RPM_PKGNAME)-$(RPM_VERSION)/ -o $(RPM_TOPDIR)/SOURCES/$(RPM_PKGNAME)-$(RPM_VERSION).tar.gz HEAD
	echo '%define pkg_version $(RPM_VERSION)' > $(RPM_GENERATED_SPEC)
	cat $(RPM_PKGNAME).spec >> $(RPM_GENERATED_SPEC)
	echo | setsid rpmbuild -v -ba --sign --rmsource --clean -D 'pkg_version $(RPM_VERSION)' -D 'lib_version $(RPM_LIB_VERSION)' $(RPM_GENERATED_SPEC)
	rm -f $(RPM_GENERATED_SPEC)
	mv $(RPM_TOPDIR)/SRPMS/$(RPM_PKGNAME)-$(RPM_VERSION)-*.rpm ./
	mv $(RPM_TOPDIR)/RPMS/*/$(RPM_PKGNAME)-$(RPM_VERSION)-*.rpm ./
	mv $(RPM_TOPDIR)/RPMS/*/$(RPM_PKGNAME)-*-$(RPM_VERSION)-*.rpm ./ || true
endif

.PHONY: all build install clean rpm modules packages
