all: modules

modules: $(patsubst %.cpp, %.so, $(wildcard *.cpp))

optical.so: optical.cpp 
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $< -lwisci2c

%.so: %.cpp 
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $<

clean:
	-rm -rf *.so

.PHONY: all modules

