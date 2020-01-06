// AudioSync is the audio synchronization feature made for vidify.
//
// The objective of the module is to obtain the delay between two audio files.
// In its real usage, one of them will be the YouTube downloaded video and
// another the recorded song.
//
// The math behind it is a circular cross-correlation by using Fast Fourier
// Transforms. The output should be somewhat similar to Numpy's
// correlate(data1, data2, "full").
//
// This module is in C mainly because of speed. FFTW is the fastest Fourier
// Transformation library, and Python threading can be tricky because of the
// GIL.
//
// `cross_correlation` may be used as a standalone, regular function, but all
// other functions declared in this module are intended to be used with
// threadig, since Vidify uses a GUI and has to run this concurrently.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <fftw3.h>
#include <string.h>
#include <vidify_audiosync/audiosync.h>
#include <vidify_audiosync/cross_correlation.h>
#include <vidify_audiosync/capture/linux_capture.h>
#include <vidify_audiosync/download/linux_download.h>


// Defining the variables from global.h
volatile global_status_t global_status = IDLE_ST;
// If audiosync_abort or similar functions are called before audiosync_run,
// nothing will happen, because the mutex and conditiona are initialized
// already.
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t interval_done = PTHREAD_COND_INITIALIZER;
pthread_cond_t ffmpeg_continue = PTHREAD_COND_INITIALIZER;


// The module can be controlled externally with these basic functions. They
// expose the global status variable, which will be received from the threads
// accordingly. These functions atomically read or write the global status.
// They should only be called after audiosync_run(), since it initializes
// the mutex and more.
void audiosync_abort() {
    pthread_mutex_lock(&mutex);
    global_status = ABORT_ST;
    pthread_mutex_unlock(&mutex);
}
void audiosync_pause() {
    pthread_mutex_lock(&mutex);
    global_status = PAUSED_ST;
    pthread_mutex_unlock(&mutex);
}
void audiosync_resume() {
    pthread_mutex_lock(&mutex);
    // Changes the global status and sends a signal to the ffmpeg threads
    // that will be waiting.
    global_status = RUNNING_ST;
    pthread_cond_broadcast(&ffmpeg_continue);
    pthread_mutex_unlock(&mutex);
}
global_status_t audiosync_status() {
    global_status_t ret;
    pthread_mutex_lock(&mutex);
    ret = global_status;
    pthread_mutex_unlock(&mutex);

    return ret;
}


// This function starts the algorithm. Only one audiosync thread can be
// running at once.
int audiosync_run(char *yt_title, long int *lag) {
    int ret = -1;
    // The audio data.
    double *cap_sample = NULL;
    double *yt_source = NULL;
    double confidence;

    // The algorithm will be run in these intervals. When both threads signal
    // that their interval is finished, the cross correlation will be
    // calculated. If it's accepted, the threads will finish and the main
    // function will return the lag calculated.
    const size_t intervals[] = {
        3 * SAMPLE_RATE,  // 144,000 frames
        6 * SAMPLE_RATE,  // 288,000 frames
        10 * SAMPLE_RATE,  // 432,000 frames
        15 * SAMPLE_RATE,  // 576,000 frames
        20 * SAMPLE_RATE,  // 720,000 frames
        30 * SAMPLE_RATE,  // 1,440,000 frames
    };
    const size_t n_intervals = sizeof(intervals) / sizeof(intervals[0]);
    const size_t length = intervals[n_intervals-1];

    // Allocated using malloc because the stack doesn't have enough memory.
    cap_sample = malloc(length * sizeof(*cap_sample));
    if (cap_sample == NULL) {
        perror("audiosync: cap_sample malloc error");
        goto finish;
    }
    yt_source = malloc(length * sizeof(*yt_source));
    if (yt_source == NULL) {
        perror("audiosync: yt_source malloc error");
        goto finish;
    }

    // Launching the threads and initializing the mutex and condition
    // variables.
    pthread_t down_th, cap_th;
    struct ffmpeg_data cap_args = {
        .title = yt_title,
        .buf = cap_sample,
        .total_len = length,
        .len = 0,
        .intervals = intervals,
        .n_intervals = n_intervals,
    };
    struct ffmpeg_data down_args = {
        .title = yt_title,
        .buf = yt_source,
        .total_len = length,
        .len = 0,
        .intervals = intervals,
        .n_intervals = n_intervals,
    };
    if (pthread_create(&cap_th, NULL, &capture, (void *) &cap_args) < 0) {
        perror("audiosync: pthread_create error");
        audiosync_abort();
        goto finish;
    }
    if (pthread_create(&down_th, NULL, &download, (void *) &down_args) < 0) {
        perror("audiosync: pthread_create error");
        audiosync_abort();
        goto finish;
    }

    // A boolean to know after the main loop that the algorithm obtained a
    // valid result.
    int success = 0;
    // The main loop iterates through all intervals until a valid result is
    // found.
    fprintf(stderr, "audiosync: starting interval loop\n");
    for (size_t i = 0; i < n_intervals; ++i) {
        // Waits for both threads to finish their interval.
        pthread_mutex_lock(&mutex);
        while (cap_args.len < intervals[i] || down_args.len < intervals[i]) {
            pthread_cond_wait(&interval_done, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        fprintf(stderr, "audiosync: Next interval (%ld): cap=%ld down=%ld\n",
                i, cap_args.len, down_args.len);

        // Running the cross correlation algorithm and checking for errors.
        if (cross_correlation(yt_source, cap_sample, intervals[i], lag,
                              &confidence) < 0) {
            continue;
        }

        // If the returned confidence is higher or equal than the minimum
        // required, the program ends with the obtained result.
        *lag = round((double) (*lag) * FRAMES_TO_MS);
        if (confidence >= MIN_CONFIDENCE) {
            // Indicating the threads to finish, and waiting for them to
            // finish safely.
            success = 1;
            audiosync_abort();
            if (pthread_join(cap_th, NULL) < 0) {
                perror("audiosync: pthread_join error");
                goto finish;
            }
            if (pthread_join(down_th, NULL) < 0) {
                perror("audiosync: pthread_join");
                goto finish;
            }
            break;
        }
    }

    // If the success flag is still at zero, it means that no result from
    // the cross_correlation function returned a valid value.
    if (!success) {
        goto finish;
    }

    // If it got here, it means that no errors happened and that a valid
    // lag was found.
    ret = 0;

finish:
    if (cap_sample) free(cap_sample);
    if (yt_source) free(yt_source);
    // Resetting the global status.
    global_status = IDLE_ST;

    return ret;
}
