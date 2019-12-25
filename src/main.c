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
#include <sndfile.h>
#include <fftw3.h>
#include "cross_correlation.h"


// Sample rate used. It has to be 48000 because most YouTube videos can only
// be downloaded at that rate, and both audio files must have the same one.
#define SAMPLE_RATE 48000


// Reading a WAV file with libsndfile, won't be used in the final version
// so it's not important.
static void read_wav(char *name, double *data, size_t length) {
    // Loading the WAV file
    SNDFILE *file;
    SF_INFO file_info;
    file = sf_open(name, SFM_READ, &file_info);
    sf_read_double (file, data, length);

    // Zero padding
    for (int i = length/2; i < length; ++i)
        data[i] = 0;
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s file1.wav file2.wav MS\n", argv[0]);
        exit(1);
    }

    // The length in milliseconds will also be provided as a parameter.
    const int ms = atoi(argv[3]);
    // Actual size of the data, having in account the sample rate and that
    // half of its size will be zero-padded.
    const size_t length = SAMPLE_RATE * (ms / 1000) * 2;
    // Reading both files. This doesn't have to be concurrent because in
    // the future this module will recieve the data arrays directly. The
    // arrays are initialized with FFTW's method because cross_correlation
    // uses this library.
    double *out1 = fftw_alloc_real(length);
    double *out2 = fftw_alloc_real(length);
    read_wav(argv[1], out1, length);
    read_wav(argv[2], out2, length);

    double lag, confidence;
    int err = cross_correlation(out1, out2, length, &lag, &confidence);
    /* printf("%f ms of delay with a confidence of %f\n", lag, confidence); */

    fftw_free(out1);
    fftw_free(out2);

    return err < 0 ? 1 : 0;
}
