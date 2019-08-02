ifndef PETA_ROOTFS
	$(error Please define the PETA_ROOTFS variable as a path to an extracted petalinux rootfs.)
endif

CFLAGS= -fomit-frame-pointer -pipe -fno-common -fno-builtin \
	-Wall \
	-march=armv7-a -mfpu=neon \
	-mthumb-interwork -mtune=cortex-a9 \
	-DEMBED -Dlinux -D__linux__ -Dunix -fPIC \
	-std=c++11 \
	-I$(PETA_ROOTFS)/usr/include \
	-I$(PETA_ROOTFS)/include

LDLIBS= -L$(PETA_ROOTFS)/stage/lib \
	-L$(PETA_ROOTFS)/stage/usr/lib

CXX=arm-linux-gnueabihf-g++

all: modules

modules: $(patsubst %.cpp, %.so, $(wildcard *.cpp))

optical.so: optical.cpp 
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

%.so: %.cpp 
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $< -lmemsvc

clean:
	-rm -rf *.so

.PHONY: all modules

