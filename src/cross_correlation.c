#ifdef DEBUG
# define _GNU_SOURCE  // Debug mode uses popen, order matters
# include <time.h>  // Debug mode uses timers
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <complex.h>
#include <fftw3.h>
#include <vidify_audiosync/audiosync.h>


// Data structure used to pass parameters to concurrent FFTW-related functions.
struct fftw_data {
    double *real;
    double complex *cpx;
    size_t length;
    pthread_mutex_t *mutex;
};


// Concurrent implementation of the Fast Fourier Transform using FFTW.
static void *fft(void *arg) {
    // Getting the parameters passed to this thread
    struct fftw_data *data = arg;

    // Initializing the plan: the only thread-safe call in FFTW is
    // fftw_execute, so the plan has to be created and destroyed with a lock.
    pthread_mutex_lock(data->mutex);
    fftw_plan p = fftw_plan_dft_r2c_1d(data->length, data->real, data->cpx,
                                       FFTW_ESTIMATE);
    pthread_mutex_unlock(data->mutex);

    // Actually executing the FFT
    fftw_execute(p);

    // Destroying the plan and terminating the thread
    pthread_mutex_lock(data->mutex);
    fftw_destroy_plan(p);
    pthread_mutex_unlock(data->mutex);
    pthread_exit(NULL);
}


// Calculating the cross-correlation between two signals `a` and `b`:
//     xcross = ifft(fft(a) * conj(fft(b)))
//
// The source size must be twice the sample size. This is because the sample
// will be zero-padded to length 2N-1, which is needed to calculate the
// circular cross-correlation.
//
// FFTW won't overwrite the source if FFTW_ESTIMATE is used, so the source
// should be initialized with fftw_alloc_real so that the data is aligned
// and thus, the Fourier Transforms will be faster.
//
// Returns the lag in frames the sample has over the source, with a confidence
// between -1 and 1.
//
// In case of error, the function returns -1.
int cross_correlation(double *source, double *input_sample,
                      const size_t sample_length, long int *displacement,
                      double *coefficient) {
    double *sample = NULL;
    double *results = NULL;
    double complex *arr1 = NULL;
    double complex *arr2 = NULL;
    const size_t length = sample_length * 2;
    int ret = -1;

    // Only the sample needs to be zero-padded, since the cross correlation
    // will be circular, and only one of the inputs is shifted.
    // FFTW doesn't overwrite the source when FFTW_ESTIMATE is used, so the
    // sample doesn't have to be copied.
    // Note: fftw_alloc_* uses fftw_malloc, which is an equivalent of running
    // malloc + memalign. This means that it may also return NULL in case of
    // error.
    sample = fftw_alloc_real(length);
    if (sample == NULL) {
        perror("audiosync: sample fftw_alloc_real failed");
        goto finish;
    }
    memcpy(sample, input_sample, sample_length * sizeof(*sample));
    memset(sample + sample_length, 0,
           (length - sample_length) * sizeof(*sample));

#ifdef DEBUG
    // Plotting the output with gnuplot
    fprintf(stderr, "audiosync: Saving initial plot to '%ld_original.png'\n",
            length);
    FILE *gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "set term 'png'\n");
    fprintf(gnuplot, "set output 'images/%ld_original.png'\n", length);
    fprintf(gnuplot, "plot '-' with lines title 'sample', '-' with lines"
            " title 'source'\n");
    for (size_t i = 0; i < length; ++i)
        fprintf(gnuplot, "%f\n", source[i]);
    fprintf(gnuplot, "e\n");
    for (size_t i = 0; i < length; ++i)
        fprintf(gnuplot, "%f\n", sample[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);

    // Benchmarking the matching function
    clock_t start = clock();
#endif

    // Getting the complex results from both FFT. The output length for the
    // complex numbers is n/2+1.
    const size_t cpx_length = (length / 2) + 1;
    arr1 = fftw_alloc_complex(cpx_length);
    if (arr1 == NULL) {
        perror("audiosync: arr1 fftw_alloc_real failed");
        goto finish;
    }
    arr2 = fftw_alloc_complex(cpx_length);
    if (arr2 == NULL) {
        perror("audiosync: arr2 fftw_alloc_real failed");
        goto finish;
    }
    results = fftw_alloc_real(length);
    if (results == NULL) {
        perror("audiosync: results fftw_alloc_real failed");
        goto finish;
    }

    // Initializing the threads and running them
    pthread_mutex_t fft_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t fft1_th, fft2_th;
    struct fftw_data fft1_data = {
        .real = source,
        .cpx = arr1,
        .length = length,
        .mutex = &fft_mutex
    };
    struct fftw_data fft2_data = {
        .real = sample,
        .cpx = arr2,
        .length = length,
        .mutex = &fft_mutex
    };
    if (pthread_create(&fft1_th, NULL, &fft, (void *) &fft1_data) < 0) {
        perror("audiosync: pthread_create for fft1_th failed");
        goto finish;
    }
    if (pthread_create(&fft2_th, NULL, &fft, (void *) &fft2_data) < 0) {
        perror("audiosync: pthread_create for fft2_th failed");
        goto finish;
    }
    if (pthread_join(fft1_th, NULL) < 0) {
        perror("audiosync: pthread_join for fft1_th failed");
        goto finish;
    }
    if (pthread_join(fft2_th, NULL) < 0) {
        perror("audiosync: pthread_join for fft2_th failed");
        goto finish;
    }

    // Product of fft1 * conj(fft2), saved in the first array.
    for (size_t i = 0; i < cpx_length; ++i)
        arr1[i] *= conj(arr2[i]);

    // And calculating the ifft. The size of the results is going to be the
    // original length again.
    fftw_plan p = fftw_plan_dft_c2r_1d(length, arr1, results, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);

    // Getting the results: the index of the maximum value is the desired lag.
    double abs_result;
    double max = results[0];
    long int lag = 0;
    for (size_t i = 1; i < length; ++i) {
        abs_result = fabs(results[i]);
        if (abs_result > max) {
            max = abs_result;
            lag = i;
        }
    }

    // If the lag is greater than the input array itself, it means that the
    // displacement that has to be performed is to the left, and otherwise
    // to the right.
    size_t shift_start = 0;
    size_t shift_end = sample_length;
    if (lag >= (long int) sample_length) {
        // Performing the displacement to the left
        lag = (long int) sample_length - (lag % (long int) sample_length);
        shift_end = (sample_length - lag);
        memmove(sample, sample + lag, sizeof(double) * shift_end);
        *displacement = -lag;
    } else {
        // Performing the displacement to the right
        shift_start = lag;
        memmove(sample + shift_start, sample, sizeof(double) * shift_end);
        *displacement = lag;
    }

    // Calculating the Pearson Correlation Coefficient applying the formula:
    // https://en.wikipedia.org/wiki/Pearson_correlation_coefficient#For_a_sample
    // 1. The average for both datasets.
    double sum1 = 0.0;
    double sum2 = 0.0;
    for (size_t i = shift_start; i < shift_end; ++i) {
         sum1 += source[i];
         sum2 += sample[i];
    }
    double avg1 = sum1 / (shift_end - shift_start);
    double avg2 = sum2 / (shift_end - shift_start);
    // 2. Applying the definition formula.
    double diff1, diff2;
    double diffprod = 0.0;
    double diff1_squared = 0.0;
    double diff2_squared = 0.0;
    for (size_t i = shift_start; i < shift_end; ++i) {
        diff1 = source[i] - avg1;
        diff2 = sample[i] - avg2;
        diffprod += diff1 * diff2;
        diff1_squared += diff1 * diff1;
        diff2_squared += diff2 * diff2;
    }
    *coefficient = diffprod / sqrt(diff1_squared * diff2_squared);

    // Checking that the resulting coefficient isn't NaN
    if (*coefficient != *coefficient) goto finish;

    fprintf(stderr, "audiosync: %ld frames of delay with a confidence of %f\n",
            *displacement, *coefficient);

#ifdef DEBUG
    fprintf(stderr, "audiosync: Result obtained in %f secs\n",
            (clock() - start) / (double) CLOCKS_PER_SEC);

    // Plotting the output with gnuplot
    fprintf(stderr, "audiosync: Saving plot to '%ld.png'\n", length);
    gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "set term 'png'\n");
    fprintf(gnuplot, "set output 'images/%ld.png'\n", length);
    fprintf(gnuplot, "plot '-' with lines title 'sample', '-' with lines"
            " title 'source'\n");
    for (size_t i = shift_start; i < shift_end; ++i)
        fprintf(gnuplot, "%f\n", source[i]);
    fprintf(gnuplot, "e\n");
    for (size_t i = shift_start; i < shift_end; ++i)
        fprintf(gnuplot, "%f\n", sample[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);
#endif

    ret = 0;

finish:
    if (sample) fftw_free(sample);
    if (arr1) fftw_free(arr1);
    if (arr2) fftw_free(arr2);
    if (results) fftw_free(results);

    return ret;
}
