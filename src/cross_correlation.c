#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fftw3.h>
#include <math.h>


// fftw_complex indexes for clarification.
#define FFTW_REAL 0
#define FFTW_IMAG 1


// Global mutex used for multithreading.
static pthread_mutex_t MUTEX;

// Data structure used to pass parameters to concurrent FFTW-related functions.
struct fftw_data {
    double *real;
    fftw_complex *cpx;
    size_t length;
};


// Calculating the magnitude of a complex number.
static inline double get_magnitude(double r, double i) {
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

    // Actually executing the IFFT
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
// TODO: Better error handling
//
// Returns the delay in samples the second data set has over the first
// one, with a confidence between 0 and 1.
int cross_correlation(double *data1, double *data2, const size_t length,
                      double *confidence) {
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
    double magnitude;
    for (size_t i = 0; i < cpx_length; ++i) {
        magnitude = get_magnitude(out2[i][FFTW_REAL], out2[i][FFTW_IMAG]);
        in[i][FFTW_REAL] = out1[i][FFTW_REAL] * magnitude;
        in[i][FFTW_IMAG] = out1[i][FFTW_IMAG] * magnitude;
    }
    // The size of the results is going to be the original length again.
    double *results = fftw_alloc_real(length);

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

    return delay;
}


