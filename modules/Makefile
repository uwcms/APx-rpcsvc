ifndef PETA_ROOTFS
$(error Please define the PETA_ROOTFS variable as a path to an extracted petalinux rootfs.)
endif

CFLAGS= \
		--sysroot=$(PETA_ROOTFS) \
		-fomit-frame-pointer -pipe -fno-common -fno-builtin \
		-Wall \
		-march=armv7-a -mfpu=neon \
		-mthumb-interwork -mtune=cortex-a9 \
		-DEMBED -Dlinux -D__linux__ -Dunix -fPIC \
		-std=c++11 \
		-I$(PETA_ROOTFS)/usr/include \
		-I$(PETA_ROOTFS)/include \
		-I..

LDLIBS= \
		-L$(PETA_ROOTFS)/lib \
		-L$(PETA_ROOTFS)/usr/lib

CXX=arm-linux-gnueabihf-g++

all: modules

modules: $(patsubst %.cpp, %.so, $(wildcard *.cpp))

optical.so: optical.cpp 
	$(CXX) $(CFLAGS) $(LDLIBS) -fPIC -shared -o $@ $< -lwisci2c

%.so: %.cpp 
	$(CXX) $(CFLAGS) $(LDLIBS) -fPIC -shared -o $@ $< -l:libmemsvc.so.1

clean:
	-rm -rf *.so

.PHONY: all modules

