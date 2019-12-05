CC = g++
FLAGS = -std=c++11 -Wall -pthread -lfftw3 -lm

all: main

# Debug mode:
# Enables GDB debug (-g), AddressSanitizer and plots with matplotlib.
# Benchmarks shouldn't be taken into account when using debug mode because
# it increases a regular calculation from .06 seconds to an entire second.
# Use release for that instead, which is what's going to be used when
# it's finished.
debug: FLAGS += -DPLOT -DDEBUG -I/usr/include/python3.8 -I/usr/lib/python3.8/site-packages/numpy/core/include -lpython3.8
debug: main

# Release mode:
# For now it just enables optimization (-DDEBUG will be removed).
release: FLAGS += -O3 -DDEBUG
release: main

main: main.o
	$(CC) $(FLAGS) main.o -o main

main.o: src/main.cpp
	$(CC) $(FLAGS) -c src/main.cpp

clean:
	rm -f main *.o
