CC = gcc
CFLAGS = -Wall -Wextra -g3 -std=c11 -DDEBUG -fsanitize=undefined
LDLIBS = -lm -pthread -lfftw3
OBJS = example.o audiosync.o cross_correlation.o ffmpeg_pipe.o linux_capture.o linux_download.o

all: example

example: $(OBJS)
	$(CC) $(CFLAGS) -o example $(OBJS) $(LDLIBS)

example.o: example.c
	$(CC) $(CFLAGS) -c example.c

audiosync.o: ../src/audiosync.c
	$(CC) $(CFLAGS) -c ../src/audiosync.c

cross_correlation.o: ../src/cross_correlation.c
	$(CC) $(CFLAGS) -c ../src/cross_correlation.c

ffmpeg_pipe.o: ../src/ffmpeg_pipe.c
	$(CC) $(CFLAGS) -c ../src/ffmpeg_pipe.c

linux_capture.o: ../src/capture/linux_capture.c
	$(CC) $(CFLAGS) -c ../src/capture/linux_capture.c

linux_download.o: ../src/download/linux_download.c
	$(CC) $(CFLAGS) -c ../src/download/linux_download.c

clean:
	rm -f *.o example
