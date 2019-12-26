// AudioSync is the audio synchronization feature made for spotivids.
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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fftw3.h>
#include "global.h"
#include "cross_correlation.h"
#include "capture/linux_capture.h"
#include "download/linux_download.h"


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s URL\n", argv[0]);
        exit(1);
    }

    // The algorithm will be run in these intervals. When both threads signal
    // that their interval is finished, the cross correlation will be
    // calculated. If it's accepted, the threads will finish and the main
    // function will return the lag calculated.
    const int n_intervals = 5;
    int *intervals;
    intervals = malloc(sizeof(int) * n_intervals);
    if (intervals == NULL) {
        perror("malloc");
        exit(1);
    }
    // Intervals in frames
    intervals[0] = 3 *  SAMPLE_RATE;  // 144000 frames
    intervals[1] = 6 *  SAMPLE_RATE;  // 288000 frames
    intervals[2] = 9 *  SAMPLE_RATE;  // 432000 frames
    intervals[3] = 12 * SAMPLE_RATE;  // 576000 frames
    intervals[4] = 15 * SAMPLE_RATE;  // 720000 frames
    pthread_mutex_t global_mutex;
    pthread_cond_t thread_done;

    // Actual size of the data, having in account the sample rate and that
    // half of its size will be zero-padded.
    const size_t length = intervals[n_intervals-1];
    int err = 1;

    // The arrays are initialized with FFTW's method because cross_correlation
    // uses this library.
    double *out1 = fftw_alloc_real(length);
    double *out2 = fftw_alloc_real(length);

    // Launching the threads
    pthread_t down_th, cap_th;
    struct down_data down = {
        .url = argv[1],
        .buf = out1,
        .total_len = length,
        .len = 0,
        .intervals = intervals,
        .n_intervals = n_intervals,
        .mutex = &global_mutex,
        .done = &thread_done
    };
    struct cap_data cap = {
        .buf = out2,
        .total_len = length,
        .len = 0,
        .intervals = intervals,
        .n_intervals = n_intervals,
        .mutex = &global_mutex,
        .done = &thread_done
    };
    if (pthread_create(&cap_th, NULL, &capture, (void *) &cap)) {
        perror("pthread_create");
        goto finish;
    }
    if (pthread_create(&down_th, NULL, &download, (void *) &down)) {
        perror("pthread_create");
        goto finish;
    }

    double lag, confidence;
    for (int i = 0; i < n_intervals; ++i) {
        // Waits for both threads to finish their interval.
        while (cap.len < intervals[i] || down.len < intervals[i]) {
            pthread_cond_wait(&thread_done, &global_mutex);
        }
        printf(">> Next interval (%d): cap=%ld down=%ld\n", i, cap.len, down.len);

        // Runs the cross correlation. If the results have a good enough
        // confidence, the function ends and returns the obtained value.
        if (cross_correlation(out1, out2, intervals[i], &lag, &confidence) < 0) {
            continue;
        }
        if (confidence > MIN_CONFIDENCE) {
            // Warning, pthread_cancel is aggressive.
            pthread_cancel(down_th);
            pthread_cancel(cap_th);
            break;
        }
    }


finish:
    if (out1) fftw_free(out1);
    if (out2) fftw_free(out2);

    return err;
}
