CC = gcc
# -fsanitize=address is disabled until https://github.com/FFTW/fftw3/issues/186
# is resolved.
CFLAGS = -Wall -g -Wextra -std=c11 -DDEBUG -fsanitize=undefined -fsanitize=address
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
