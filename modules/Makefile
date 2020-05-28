all: modules

modules: $(patsubst %.cpp, %.so, $(wildcard *.cpp))

%.so: %.cpp
	$(CXX) $(CFLAGS) $(LDFLAGS) -fPIC -shared -o $@ $<

clean:
	-rm -rf *.so

.PHONY: all modules

