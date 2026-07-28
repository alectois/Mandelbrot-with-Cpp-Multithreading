CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Wpedantic -Wconversion
THREAD_FLAGS ?= -pthread

.PHONY: all test clean

all: mandelbrot mandelbrot-parallel

mandelbrot: mandelbrot.cpp mandelbrot-common.hpp
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) mandelbrot.cpp -o $@

mandelbrot-parallel: mandelbrot-parallel.cpp mandelbrot-common.hpp
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) mandelbrot-parallel.cpp -o $@

test: all
	./mandelbrot --height 72 --width 96 --max-iterations 256 --no-output
	./mandelbrot-parallel --height 72 --width 96 --max-iterations 256 \
		--num-threads 4 --work-allocation static --verify --no-output
	./mandelbrot-parallel --height 72 --width 96 --max-iterations 256 \
		--num-threads 4 --work-allocation dynamic --verify --no-output

clean:
	rm -f mandelbrot mandelbrot-parallel mandelbrot.ppm
