.DEFAULT_GOAL = all

.PHONY: all debug release clean

CXX = g++
CXXFLAGS = -Wall -Werror -std=c++17 -fPIC

-include $(patsubst %.cc, build/deps/%.d, $(wildcard *.cc))

all: debug

debug: CXXFLAGS += -g
debug: build/vm.so

release: CXXFLAGS += -O2
release: build/vm.so

build/deps/%.d: %.cc | build/deps
	@$(CXX) $(CXXFLAGS) -E -MMD -MT build/$*.o -MF $@ $< > /dev/null 2>&1

build/%.o: %.cc | build
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/vm.so: $(patsubst %.cc, build/%.o, $(wildcard *.cc))
	$(CXX) $(CXXFLAGS) -shared -o $@ $^

build:
	mkdir -p $@
build/deps:
	mkdir -p $@

clean:
	rm -rf build
