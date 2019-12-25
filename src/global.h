#ifndef _H_DEFINES
#define _H_DEFINES

#include <stdlib.h>
#include <pthread.h>

// Sample rate used. It has to be 48000 because most YouTube videos can only
// be downloaded at that rate, and both audio files must have the same one.
#define SAMPLE_RATE 48000
#define SAMPLE_RATE_STR "48000"
#define NUM_CHANNELS 1
#define NUM_CHANNELS_STR "1"

// Conversion from the used sample rate to milliseconds: 48000 / 1000
#define SAMPLES_TO_MS 48

#define MIN_CONFIDENCE 0.75

// Struct used to pass variables to pthreads.
struct down_data {
    char *url;
    double *buf;
    size_t total_len;
    size_t len;
    const int *intervals;
    const int n_intervals;
    pthread_mutex_t *mutex;
    pthread_cond_t *done;
};
struct cap_data {
    double *buf;
    size_t total_len;
    size_t len;
    const int *intervals;
    const int n_intervals;
    pthread_mutex_t *mutex;
    pthread_cond_t *done;
};

#endif /* _H_DEFINES */
