.DEFAULT_GOAL = all

.PHONY: venv all debug release vm clean

all: debug

venv:
	python3 -m venv env/python

debug: BUILD_TYPE = debug
debug: vm

release: BUILD_TYPE = release
release: vm

vm:
	make -C vm $(BUILD_TYPE)

clean:
	make -C vm clean
