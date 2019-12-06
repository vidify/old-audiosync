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
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <fftw3.h>
#include <sndfile.h>

// Global mutex used for multithreading.
pthread_mutex_t MUTEX;
// Sample rate used. It has to be 48000 because most YouTube videos can only
// be downloaded at that rate, and both audio files must have the same one.
#define SAMPLE_RATE 48000
// Conversion from the used sample rate to milliseconds: 48000 / 1000
#define SAMPLES_TO_MS 48
// The format used for the samples.
typedef double sample_t;

// Data structure used to pass parameters to concurrent FFTW-related functions.
struct fftw_data {
    sample_t *real;
    fftw_complex *cpx;
    size_t length;
};

// fftw_complex indexes for clarification.
#define REAL 0
#define IMAG 1


// Calculating the magnitude of a complex number.
static inline sample_t getMagnitude(sample_t r, sample_t i) {
    return sqrt(r*r + i*i);
}


// Concurrent implementation of the Fast Fourier Transform using FFTW.
static void *fft(void *thread_arg) {
    // Getting the parameters passed to this thread
    struct fftw_data *data;
    data = (struct fftw_data *) thread_arg;

    // Initializing the plan: the only thread-safe call in FFTW is
    // fftw_execute, so the plan has to be created and destroyed with
    // a lock.
    pthread_mutex_lock(&MUTEX);
    fftw_plan p = fftw_plan_dft_r2c_1d(data->length, data->real, data->cpx, FFTW_ESTIMATE);
    pthread_mutex_unlock(&MUTEX);

    // Actually executing the FFT
    fftw_execute(p);

    // Destroying the plan and terminate the thread
    pthread_mutex_lock(&MUTEX);
    fftw_destroy_plan(p);
    pthread_mutex_unlock(&MUTEX);
    pthread_exit(NULL);
}


// The Inverse Fast Fourier Transform using FFTW.
static void *ifft(void *thread_arg) {
    // Getting the parameters passed to this thread
    struct fftw_data *data;
    data = (struct fftw_data *) thread_arg;

    // Initializing the plan: the only thread-safe call in FFTW is
    // fftw_execute, so the plan has to be created and destroyed with
    // a lock.
    pthread_mutex_lock(&MUTEX);
    fftw_plan p = fftw_plan_dft_c2r_1d(data->length, data->cpx, data->real, FFTW_ESTIMATE);
    pthread_mutex_unlock(&MUTEX);

    // Actually executing the FFT
    fftw_execute(p);

    // Destroying the plan and terminate the thread
    pthread_mutex_lock(&MUTEX);
    fftw_destroy_plan(p);
    pthread_mutex_unlock(&MUTEX);
    pthread_exit(NULL);
}


// Calculating the cross-correlation between two signals `a` and `b`:
//     xcross = ifft(fft(a) * magn(fft(b)))
// And where magn() is the magnitude of the complex numbers returned by the
// fft. Instead of magn(), a reverse() function could be used. But the
// magnitude seems easier to calculate for now.
//
// Both datasets should have the same size. They should be zero-padded to
// length 2N-1. This is needed to calculate the circular cross-correlation
// rather than the regular cross-correlation.
//
// TODO: Get confidence level: https://dsp.stackexchange.com/questions/9797/cross-correlation-peak
// TODO: All functions in this module should also return an error code in case
// something doesn't go right.
//
// Returns the delay in milliseconds the second data set has over the first
// one, with a confidence between 0 and 1.
double cross_correlation(sample_t *data1, sample_t *data2,
                         const size_t length, double *confidence) {
    // Getting the complex results from both FFT. The output length for the
    // complex numbers is n/2+1.
    const size_t cpx_length = (length / 2) + 1;
    fftw_complex *out1 = fftw_alloc_complex(cpx_length);
    fftw_complex *out2 = fftw_alloc_complex(cpx_length);

    // Initializing the threads and running them
    int err;
    pthread_t fft1_thread, fft2_thread;
    struct fftw_data fft1_data = {
        .real = data1,
        .cpx = out1,
        .length = length
    };
    struct fftw_data fft2_data = {
        .real = data2,
        .cpx = out2,
        .length = length
    };
    err = pthread_create(&fft1_thread, NULL, &fft, (void *) &fft1_data);
    if (err) {
        fprintf(stderr, "pthread_create failed: %d\n", err);
        exit(1);
    }
    err = pthread_create(&fft2_thread, NULL, &fft, (void *) &fft2_data);
    if (err) {
        fprintf(stderr, "pthread_create failed: %d\n", err);
        exit(1);
    }
    pthread_join(fft1_thread, NULL);
    pthread_join(fft2_thread, NULL);

    // Product of fft1 * mag(fft2) where fft1 is complex and mag(fft2) is real.
    fftw_complex *in = fftw_alloc_complex(cpx_length);
    sample_t magnitude;
    for (size_t i = 0; i < cpx_length; ++i) {
        magnitude = getMagnitude(out2[i][REAL], out2[i][IMAG]);
        in[i][REAL] = out1[i][REAL] * magnitude;
        in[i][IMAG] = out1[i][IMAG] * magnitude;
    }
    // The size of the results is going to be the original length again.
    sample_t *results = fftw_alloc_real(length);

    // Concurrently executing the ifft. This is not needed right not but it
    // might when the confidence is calculated.
    pthread_t ifft_thread;
    struct fftw_data ifft_data = {
        .real = results,
        .cpx = in,
        .length = length
    };
    err = pthread_create(&ifft_thread, NULL, &ifft, (void *) &ifft_data);
    if (err) {
        fprintf(stderr, "pthread_create failed: %d\n", err);
        exit(1);
    }
    pthread_join(ifft_thread, NULL);

    // Getting the results
    *confidence = results[0];
    int delay = 0;
    for (size_t i = 1; i < length; ++i) {
        if (fabs(results[i]) > *confidence) {
            *confidence = results[i];
            delay = i;
        }
    }

#ifdef DEBUG
    printf("Plotting results\n");
    // Showing a plot of the results with gnuplot
    FILE *gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "plot '-' with dots\n");
    for (int i = 0; i < length; ++i)
        fprintf(gnuplot, "%f\n", results[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    printf("Press enter to continue\n");
    getchar();
    pclose(gnuplot);
#endif

    fftw_free(out1);
    fftw_free(out2);
    fftw_free(in);
    fftw_free(results);

    // Conversion to milliseconds with 48000KHz as the sample rate.
    return delay / SAMPLES_TO_MS;
}


// Reading a WAV file with libsndfile, won't be used in the final version
// so it's not important.
static void read_wav(char *name, sample_t *data, size_t length) {
    // Loading the WAV file
    SNDFILE *file;
    SF_INFO file_info;
    file = sf_open(name, SFM_READ, &file_info);

    sf_count_t items;
    items = sf_read_double (file, data, length);
#ifdef DEBUG
    printf("%ld items read\n", items);
#endif
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file1>.wav <file2>.wav\n", argv[0]);
        exit(1);
    }

    // The length in milliseconds will also be provided as a parameter.
    const int ms = 5000;
    // Actual size of the data, having in account the sample rate and that
    // half of its size will be zero-padded.
    const int length = SAMPLE_RATE * (ms / 1000) * 2;
    // Reading both files. This doesn't have to be concurrent because in
    // the future this module will recieve the data arrays directly.
    sample_t *out1 = fftw_alloc_real(length);
    sample_t *out2 = fftw_alloc_real(length);
    read_wav(argv[1], out1, length);
    read_wav(argv[2], out2, length);

#ifdef DEBUG
    printf("WAV files read correctly\n");
    // Benchmarking the matching function
    clock_t start = clock();
#endif

    double confidence;
    double delay = cross_correlation(out1, out2, length, &confidence);
    printf("%f ms of delay with a confidence of %f\n", delay, confidence);

#ifdef DEBUG
    double duration = (clock() - start) / (double) CLOCKS_PER_SEC;
    printf("Matching took %fs\n", duration);
    double samplesDelay = delay * SAMPLES_TO_MS;

    // Plotting the output with gnuplot
    printf("Plotting fixed audio tracks\n");
    FILE *gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "plot '-' with dots, '-' with dots\n");
    for (int i = 0; i < length; ++i)
        fprintf(gnuplot, "%f\n", out1[i]);
    fprintf(gnuplot, "e\n");
    // The second audio file starts at samplesDelay
    for (int i = samplesDelay; i < length; ++i)
        fprintf(gnuplot, "%f\n", out2[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    printf("Press enter to continue\n");
    getchar();
    pclose(gnuplot);
#endif

    fftw_free(out1);
    fftw_free(out2);

    return 0;
}
