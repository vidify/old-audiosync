CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -DDEBUG -fsanitize=address -fsanitize=undefined
LDLIBS = -lm -pthread -lfftw3
OBJS = tests.o cross_correlation.o

all: tests

tests: $(OBJS)
	$(CC) $(CFLAGS) -o tests $(OBJS) $(LDLIBS)

tests.o: tests.c
	$(CC) $(CFLAGS) -c tests.c

cross_correlation.o: ../src/cross_correlation.c
	$(CC) $(CFLAGS) -c ../src/cross_correlation.c

clean:
	rm -f cross_correlation.o tests.o tests
