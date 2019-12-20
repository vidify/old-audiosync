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
#include <time.h>
#include <fftw3.h>
#include <sndfile.h>

// Recording audio multi-platform
#if defined(__linux__)
#include "capture_audio/linux_capture.h"
#elif defined(_WIN32)
#include "capture_audio/windows_capture.h"
#else
#error "Platform not supported"
#endif
// Downloading the video multi-platform
#include "download.h"

#include "cross_correlation.h"

// Both audio tracks should be mono.
#define NUM_CHANNELS 1
#define NUM_CHANNELS_STR "1"
// Sample rate used. It has to be 48000 because most YouTube videos can only
// be downloaded at that rate, and both audio files must have the same one.
#define SAMPLE_RATE 48000
#define SAMPLE_RATE_STR "48000"


// Zero padding an array: making half an array's size filled with zeroes.
// Its total 2N-1 size should already be allocated.
static inline void zero_pad(double *data, size_t length) {
    for (int i = length/2; i < length; ++i)
        data[i] = 0;
}


// Reading a WAV file with libsndfile, won't be used in the final version
// so it's not important.
/*static void read_wav(char *name, double *data, size_t length) {
    // Loading the WAV file
    SNDFILE *file;
    SF_INFO file_info;
    file = sf_open(name, SFM_READ, &file_info);

    sf_count_t items;
    items = sf_read_double (file, data, length);

    sf_close(file);

#ifdef DEBUG
    printf("%ld items read\n", items);
#endif
}*/


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <url> <ms>\n", argv[0]);
        exit(1);
    }

    // The length in milliseconds is provided as a parameter.
    const int ms = atoi(argv[2]);
    // Actual size of the data, having in account the sample rate and that
    // half of its size will be zero-padded.
    const size_t length = 2.0 * (double) SAMPLE_RATE * ((double) ms / 1000.0);
    // Reading both files. This doesn't have to be concurrent because in
    // the future this module will recieve the data arrays directly.
    double *track1 = fftw_alloc_real(length);
    double *track2 = fftw_alloc_real(length);
    // TODO: Make these concurrent. They should always be downloading until
    // the maximum length is found (15s for example), but send some kind
    // of event signal when n% of its content are downloaded. That way,
    // the matching function could be run every 3 seconds, for example.
    // This part should also be tweaked according to the user's internet
    // speed (which is an option inside the python program itself).
    download(argv[1], track1, length, SAMPLE_RATE_STR, NUM_CHANNELS_STR);
    capture_audio(track2, length, SAMPLE_RATE, NUM_CHANNELS);

    zero_pad(track1, length);
    zero_pad(track2, length);

#ifdef DEBUG
    // Plotting the input with gnuplot
    printf("Plotting input audio tracks\n");
    FILE *gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "plot '-' with dots, '-' with dots\n");
    for (int i = 0; i < length; ++i)
        fprintf(gnuplot, "%f\n", track1[i]);
    fprintf(gnuplot, "e\n");
    for (int i = 0; i < length; ++i)
        fprintf(gnuplot, "%f\n", track2[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    printf("Press enter to continue\n");
    getchar();

    // Benchmarking the matching function
    clock_t start = clock();
#endif

    double confidence;
    double delay = cross_correlation(track1, track2, length, &confidence);
    printf("%f ms of delay with a confidence of %f\n", (delay / SAMPLE_RATE) * 1000, confidence);

#ifdef DEBUG
    double duration = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Matching took %fs\n", duration);

    // Plotting the output with gnuplot
    printf("Plotting fixed audio tracks\n");
    fprintf(gnuplot, "plot '-' with dots, '-' with dots\n");
    for (int i = 0; i < length; ++i)
        fprintf(gnuplot, "%f\n", track1[i]);
    fprintf(gnuplot, "e\n");
    // The second audio file starts at delay
    for (int i = delay; i < length; ++i)
        fprintf(gnuplot, "%f\n", track2[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    printf("Press enter to continue\n");
    getchar();
    pclose(gnuplot);
#endif

    printf("%d\n", 1);
    printf("%d\n", 2);

    return 0;
}
