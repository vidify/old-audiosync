#ifndef _H_DEFINES
#define _H_DEFINES

#include <stdlib.h>
#include <pthread.h>

// Sample rate used. It has to be 48000 because most YouTube videos can only
// be downloaded at that rate, and both audio files must have the same one.
#define NUM_CHANNELS 1
#define NUM_CHANNELS_STR "1"
#define SAMPLE_RATE 48000
#define SAMPLE_RATE_STR "48000"
// Conversion from the used sample rate to milliseconds: 48000 / 1000
#define SAMPLES_TO_MS 48

// The value of the last interval in audiosync.c in seconds.
#define MAX_SECONDS_STR "15"

// The minimum cross-correlation coefficient accepted.
#define MIN_CONFIDENCE 0.75

// The buffer size used in the capture and download modules.
#define READ_BUFSIZE 8192

// Struct used to pass variables to pthreads.
struct thread_data {
    double *buf;
    size_t total_len;
    size_t len;
    const size_t *intervals;
    const int n_intervals;
    pthread_mutex_t *mutex;
    pthread_cond_t *done;
    int *end;
};
struct down_data {
    char *url;
    struct thread_data *th_data;
};

#endif /* _H_DEFINES */
