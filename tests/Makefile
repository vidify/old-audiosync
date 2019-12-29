CC = gcc
CFLAGS = -std=c11 -Wall -lfftw3 -lm -pthread -DDEBUG -fsanitize=address -fsanitize=undefined

all: tests

tests: tests.o cross_correlation.o
	$(CC) $(CFLAGS) tests.o cross_correlation.o -o tests

tests.o: tests.c
	$(CC) $(CFLAGS) -c tests.c

cross_correlation.o: ../src/cross_correlation.c
	$(CC) $(CFLAGS) -c ../src/cross_correlation.c

clean:
	rm -f cross_correlation.o tests.o tests
