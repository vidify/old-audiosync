CC = gcc
# Dependencies: fftw3, pulseaudio and libsndfile.
# It only works on Linux for now. I'll make it available for Windows later
# on when everything is almost finished.
CFLAGS = -std=c11 -Wall -lfftw3 -lm -pthread -lpulse-simple -lpulse -lsndfile

all: main

debug: CFLAGS += -DDEBUG
debug: main

release: CFLAGS += -Ofast
release: main

main: main.o cross_correlation.o linux_capture.o
	$(CC) $(CFLAGS) main.o cross_correlation.o linux_capture.o -o main

main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c

cross_correlation.o: src/cross_correlation.c
	$(CC) $(CFLAGS) -c src/cross_correlation.c

linux_capture.o: src/capture_audio/linux_capture.c
	$(CC) $(CFLAGS) -c src/capture_audio/linux_capture.c

clean:
	rm -f main *.o
