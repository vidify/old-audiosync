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
#include <math.h>
#include <fftw3.h>
#include <string.h>
#include <vidify_audiosync/global.h>
#include <vidify_audiosync/cross_correlation.h>
#include <vidify_audiosync/capture/linux_capture.h>
#include <vidify_audiosync/download/linux_download.h>


long int get_lag(char *yt_title) {
    // The audio data.
    double *cap_sample, *yt_source;
    // Variable used to indicate the other threads to end. Any value other
    // than zero means that it should terminate. It starts at zero so that
    // if any function call fails and does a `goto finish`, the returned
    // value will be zero.
    volatile int end = 0;

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
    }
    yt_source = malloc(length * sizeof(*yt_source));
    if (yt_source == NULL) {
        perror("audiosync: yt_source malloc error");
    }

    // Launching the threads
    pthread_t down_th, cap_th;
    pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t thread_done = PTHREAD_COND_INITIALIZER;
    memset((void *) &down_th, 0, sizeof(down_th));
    memset((void *) &cap_th, 0, sizeof(cap_th));
    struct thread_data cap_params = {
        .buf = cap_sample,
        .total_len = length,
        .len = 0,
        .intervals = intervals,
        .n_intervals = n_intervals,
        .mutex = &global_mutex,
        .done = &thread_done,
        .end = &end
    };
    struct thread_data down_params = {
        .buf = yt_source,
        .total_len = length,
        .len = 0,
        .intervals = intervals,
        .n_intervals = n_intervals,
        .mutex = &global_mutex,
        .done = &thread_done,
        .end = &end
    };
    // Data structure passed to the download thread. It's a different type
    // because it also needs the url of the video to download.
    struct down_data down_th_params = {
        .yt_title = yt_title,
        .th_data = &down_params
    };
    if (pthread_create(&cap_th, NULL, &capture, (void *) &cap_params) < 0) {
        perror("audiosync: pthread_create error");
        goto finish;
    }
    if (pthread_create(&down_th, NULL, &download,
                       (void *) &down_th_params) < 0) {
        perror("audiosync: pthread_create error");
        goto finish;
    }

    long int lag;
    double confidence;
    for (size_t i = 0; i < n_intervals; ++i) {
        // Waits for both threads to finish their interval.
        pthread_mutex_lock(&global_mutex);
        while (cap_params.len < intervals[i] || down_params.len < intervals[i]) {
            pthread_cond_wait(&thread_done, &global_mutex);
        }
        pthread_mutex_unlock(&global_mutex);

        fprintf(stderr, "audiosync: Next interval (%ld): cap=%ld down=%ld\n",
                i, cap_params.len, down_params.len);

        // Running the cross correlation algorithm and checking for errors.
        if (cross_correlation(yt_source, cap_sample, intervals[i], &lag,
                              &confidence) < 0) {
            continue;
        }

        // If the returned confidence is higher or equal than the minimum
        // required, the program ends with the obtained result.
        lag = round((double) lag * FRAMES_TO_MS);
        if (confidence >= MIN_CONFIDENCE) {
            // Indicating the threads to finish, and waiting for them to
            // finish safely.
            pthread_mutex_lock(&global_mutex);
            end = 1;
            pthread_mutex_unlock(&global_mutex);
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

finish:
    if (cap_sample) free(cap_sample);
    if (yt_source) free(yt_source);

    return (end == 0) ? 0 : lag;
}
