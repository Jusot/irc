all: CMakeLists.txt | ./build
	
	cd build ; cmake .. ; make

build:
	mkdir ./build

.PHONY: clean
clean:
	rm -rf ./build/*