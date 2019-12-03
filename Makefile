CC = g++
FLAGS = -std=c++11 -Wall -pthread -lfftw3 -lm -g -I/usr/include/python3.8 -I/usr/lib/python3.8/site-packages/numpy/core/include -lpython3.8

all: main

# Plotting mode
debug: FLAGS += -DDEBUG
debug: main

main: main.o
	$(CC) $(FLAGS) main.o -o main

main.o: src/main.cpp
	$(CC) $(FLAGS) -c src/main.cpp

clean:
	rm -f main *.o

