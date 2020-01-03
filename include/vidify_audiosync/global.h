#ifndef _H_DEFINES
#define _H_DEFINES

#include <stdlib.h>
#include <pthread.h>

// Information about the audio tracks. Both must have the same formats for
// the analysis to work.
#define NUM_CHANNELS 1
#define NUM_CHANNELS_STR "1"
#define SAMPLE_RATE 48000
#define SAMPLE_RATE_STR "48000"
// Conversion factor from the WAV samples with the used sample rate to
// milliseconds.
#define FRAMES_TO_MS (1000.0 / (double) SAMPLE_RATE)

// The value of the last interval in audiosync.c in seconds. This avoids
// having to run an itoa() for intervals[n_intervals-1] and simplifies it a
// bit.
#define MAX_SECONDS_STR "30"

// The minimum cross-correlation coefficient accepted.
#define MIN_CONFIDENCE 0.85

// Structs used to pass the parameters to the threads.
struct thread_data {
    double *buf;
    size_t total_len;
    size_t len;
    const size_t *intervals;
    const size_t n_intervals;
    pthread_mutex_t *mutex;
    pthread_cond_t *done;
    int *end;
};
struct down_data {
    char *yt_title;
    struct thread_data *th_data;
};

#endif /* _H_DEFINES */
