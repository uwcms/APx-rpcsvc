PETA_STAGE=/afs/hep.wisc.edu/home/uwhepfpga/petalinux-v2014.4-final/CTP7/build/linux/rootfs/stage

CFLAGS= -fomit-frame-pointer -pipe -fno-common -fno-builtin \
	-Wall \
	-march=armv7-a -mfpu=neon -mfloat-abi=softfp \
	-mthumb-interwork -mtune=cortex-a9 \
	-DEMBED -Dlinux -D__linux__ -Dunix -fPIC \
	-I$(PETA_STAGE)/usr/include \
	-I$(PETA_STAGE)/include

LDLIBS= -L$(PETA_STAGE)/stage/lib \
	-L$(PETA_STAGE)/stage/usr/lib

CXX=arm-xilinx-linux-gnueabi-g++

all: modules

modules: $(patsubst %.cpp, %.so, $(wildcard *.cpp))

optical.so: optical.cpp 
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

%.so: %.cpp 
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $<

clean:
	-rm -rf *.so

.PHONY: all modules

