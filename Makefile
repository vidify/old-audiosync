CC = g++
FLAGS = -std=c++11 -Wall -pthread -lfftw3 -lm

all: main

# Debug mode:
# Enables GDB debug (-g), AddressSanitizer and plots with matplotlib.
# Note that AddressSanitizer raises warnings when matplotlib is used.
debug: FLAGS += -fsanitize=address -fsanitize=undefined -g -DDEBUG -I/usr/include/python3.8 -I/usr/lib/python3.8/site-packages/numpy/core/include -lpython3.8
debug: main

main: main.o
	$(CC) $(FLAGS) main.o -o main

main.o: src/main.cpp
	$(CC) $(FLAGS) -c src/main.cpp

clean:
	rm -f main *.o

