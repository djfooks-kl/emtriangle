To build

	mkdir build
	cd build
	em++ ../triangle.cpp -std=c++20 -s ALLOW_MEMORY_GROWTH=1 -s USE_WEBGL2=1 -Wall -Wextra -Wconversion -o test.html
