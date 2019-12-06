CC = gcc
# Dependencies: fftw3, pulseaudio and libsndfile.
# It only works on Linux for now. I'll make it available for Windows later
# on when everything is almost finished.
CFLAGS = -std=c11 -Wall -lfftw3 -lm -pthread -lpulse-simple -lpulse -lsndfile

all: main

# Debug mode:
# Enables GDB debug (-g), AddressSanitizer and plots with matplotlib.
# Benchmarks shouldn't be taken into account when using debug mode because
# it increases a regular calculation from .06 seconds to an entire second.
# Use release for that instead, which is what's going to be used when
# it's finished. gnuplot has to be installed for the plots to show up.
debug: CFLAGS += -DDEBUG
debug: main

# Release mode:
# For now it just enables optimization.
release: CFLAGS += -O3
release: main

main: main.o linux_capture.o
	$(CC) $(CFLAGS) main.o linux_capture.o -o main

main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c

linux_capture.o: src/capture_audio/linux_capture.c
	$(CC) $(CFLAGS) -c src/capture_audio/linux_capture.c

clean:
	rm -f main *.o
