#ifdef PLOT
# define _GNU_SOURCE  // popen() is used for plotting with gnuplot
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <complex.h>
#include <fftw3.h>
#include <audiosync/audiosync.h>


// The global cross-correlation mutex.
static pthread_mutex_t cc_mutex = PTHREAD_MUTEX_INITIALIZER;

// Data structure used to pass parameters to concurrent FFTW-related functions.
struct fftw_data {
    double *real;
    double complex *cpx;
    const size_t len;
};


// Concurrent implementation of the Fast Fourier Transform using FFTW.
static void *fft(void *arg) {
    // Getting the parameters passed to this thread
    struct fftw_data *data = arg;
    DEBUG_ASSERT(data); DEBUG_ASSERT(data->real); DEBUG_ASSERT(data->cpx);

    // Initializing the plan: the only thread-safe call in FFTW is
    // fftw_execute, so the plan has to be created and destroyed with a lock.
    pthread_mutex_lock(&cc_mutex);
    fftw_plan p = fftw_plan_dft_r2c_1d(data->len, data->real, data->cpx,
                                       FFTW_ESTIMATE);
    pthread_mutex_unlock(&cc_mutex);

    // Actually executing the FFT
    fftw_execute(p);

    // Destroying the plan and terminating the thread
    pthread_mutex_lock(&cc_mutex);
    fftw_destroy_plan(p);
    pthread_mutex_unlock(&cc_mutex);
    pthread_exit(NULL);
}

// Returns the index of the absolute maximum value in an array of doubles
// of length `len`.
//
// Its length must be greater than zero to work correctly.
static size_t max_abs_index(double *arr, size_t len) {
    DEBUG_ASSERT(arr); DEBUG_ASSERT(len > 0);

    double abs_val;
    double max_val = arr[0];
    size_t max_ind = 0;
    for (size_t i = 1; i < len; i++) {
        abs_val = fabs(arr[i]);
        if (abs_val > max_val) {
            max_val = abs_val;
            max_ind = i;
        }
    }

    return max_ind;
}

// Calculating the Pearson Correlation Coefficient between `source` and
// `sample` between two pointers, applying the formula:
// https://en.wikipedia.org/wiki/Pearson_correlation_coefficient#For_a_sample
//
// This function will only work correctly if end - start != 0.
double pearson_coefficient(double *source_start, const double *source_end,
                           double *sample_start, const double *sample_end) {
    DEBUG_ASSERT(source_start); DEBUG_ASSERT(source_end);
    DEBUG_ASSERT(source_end - source_start > 0);
    DEBUG_ASSERT(sample_start); DEBUG_ASSERT(sample_end);
    DEBUG_ASSERT(sample_end - sample_start > 0);

    // 1. The average for both datasets.
    double sum1 = 0.0;
    double sum2 = 0.0;
    // Both segments have the same size but start at different indexes.
    double *source_i = source_start;
    double *sample_i = sample_start;
    while (source_i != source_end) {
         sum1 += *source_i;
         sum2 += *sample_i;

         source_i++;
         sample_i++;
    }
    double avg1 = sum1 / (source_end - source_start);
    double avg2 = sum2 / (sample_end - sample_start);

    // 2. Applying the definition formula.
    double diff1, diff2;
    double diffprod = 0.0;
    double diff1_squared = 0.0;
    double diff2_squared = 0.0;
    source_i = source_start;
    sample_i = sample_start;
    while (source_i != source_end) {
        diff1 = *source_i - avg1;
        diff2 = *sample_i - avg2;
        diffprod += diff1 * diff2;
        diff1_squared += diff1 * diff1;
        diff2_squared += diff2 * diff2;

        source_i++;
        sample_i++;
    }

    return diffprod / sqrt(diff1_squared * diff2_squared);
}

// Calculating the cross-correlation between two signals `a` and `b`:
//     xcross = ifft(fft(a) * conj(fft(b)))
//
// The source size must be twice the sample size. This is because the sample
// will be zero-padded to length 2N-1, which is needed to calculate the
// circular cross-correlation.
//
// Returns the lag in frames the sample has over the source, with a confidence
// between -1 and 1.
//
// In case of error, the function returns -1. Otherwise, zero.
//
// Note: FFTW won't overwrite the source if FFTW_ESTIMATE is used, meaning
// that the source can be initialized with fftw_alloc_real so that it's also
// aligned and thus, the Fourier Transforms will be faster.
int cross_correlation(double *source, double *input_sample,
                      const size_t sample_len, long *lag,
                      double *coefficient) {
    DEBUG_ASSERT(source); DEBUG_ASSERT(input_sample);
    DEBUG_ASSERT(lag); DEBUG_ASSERT(coefficient);
    DEBUG_ASSERT(sample_len > 0);

    int ret = -1;
    const size_t source_len = sample_len * 2;
    const size_t cpx_len = (source_len / 2) + 1;
    double *sample = NULL;
    double *results = NULL;
    double complex *arr1 = NULL;
    double complex *arr2 = NULL;
    pthread_t fft1_th = 0;
    pthread_t fft2_th = 0;
    double *source_start, *source_end, *sample_start, *sample_end;

    // Only the sample needs to be zero-padded, since the cross correlation
    // will be circular, and only one of the inputs is shifted.
    // FFTW doesn't overwrite the source when FFTW_ESTIMATE is used, so the
    // sample doesn't have to be copied.
    //
    // Note: fftw_alloc_* uses fftw_malloc, which is an equivalent of running
    // malloc + memalign. This means that it may also return NULL in case of
    // error.
    sample = fftw_alloc_real(source_len);
    if (sample == NULL) {
        perror("audiosync: sample fftw_alloc_real failed");
        goto finish;
    }
    memcpy(sample, input_sample, sample_len * sizeof(*sample));
    memset(sample + sample_len, 0,
           (source_len - sample_len) * sizeof(*sample));

#ifdef PLOT
    // Plotting the output with gnuplot
    LOG("Saving initial plot to '%ld_original.png'", source_len);
    FILE *gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "set term 'png'\n");
    fprintf(gnuplot, "set output 'images/%ld_original.png'\n", source_len);
    fprintf(gnuplot, "plot '-' with lines title 'sample', '-' with lines"
            " title 'source'\n");
    for (size_t i = 0; i < source_len; ++i)
        fprintf(gnuplot, "%f\n", source[i]);
    fprintf(gnuplot, "e\n");
    for (size_t i = 0; i < source_len; ++i)
        fprintf(gnuplot, "%f\n", sample[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);
#endif

    // First allocating the arrays where the results will be saved at.
    arr1 = fftw_alloc_complex(cpx_len);
    if (arr1 == NULL) {
        perror("audiosync: arr1 fftw_alloc_real failed");
        goto finish;
    }
    arr2 = fftw_alloc_complex(cpx_len);
    if (arr2 == NULL) {
        perror("audiosync: arr2 fftw_alloc_real failed");
        goto finish;
    }
    results = fftw_alloc_real(source_len);
    if (results == NULL) {
        perror("audiosync: results fftw_alloc_real failed");
        goto finish;
    }

    // Initializing the threads and starting them.
    struct fftw_data fft1_data = {
        .real = source,
        .cpx = arr1,
        .len = source_len,
    };
    struct fftw_data fft2_data = {
        .real = sample,
        .cpx = arr2,
        .len = source_len,
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

    // Product of fft1 and conj(fft2), saved in the first array.
    for (size_t i = 0; i < cpx_len; ++i)
        arr1[i] *= conj(arr2[i]);

    // And calculating the ifft. The size of the results is going to be the
    // original length again.
    fftw_plan p = fftw_plan_dft_c2r_1d(source_len, arr1, results, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);

    // The index of the maximum value is the desired lag.
    *lag = max_abs_index(results, source_len);

    // If the lag is greater than the input array itself, it means that the
    // sample displacement has to be performed is to the left, and otherwise
    // to the right.
    //
    // The source size is twice the sample size, so if the sample is displaced
    // to the right, no sample data will be lost, and the resulting size will
    // be sample_len. But if the sample is moved to the left, some elements
    // will be lost from it and thus, the resulting size will be
    // sample_len - lag.
    //
    // Finally, the Pearson Correlation Coefficient is calculated with the
    // resulting segment of data.
    if (*lag >= (long) sample_len) {
        // Displacing the sample to the left (lag is negative), final size
        // is sample_len - lag.
        *lag = (*lag % (long) sample_len) - (long) sample_len;
        source_start = source;
        source_end = source + *lag + sample_len;
        sample_start = sample - *lag;
        sample_end = sample + sample_len;
    } else {
        // Displacing the sample to the right (lag is positive), final size
        // is sample_len.
        source_start = source + *lag;
        source_end = source + *lag + sample_len;
        sample_start = sample;
        sample_end = sample + sample_len;
    }
    *coefficient = pearson_coefficient(source_start, source_end, sample_start,
                                       sample_end);

    // Checking that the resulting coefficient isn't NaN.
    if (*coefficient != *coefficient) goto finish;

    LOG("%ld frames of delay with a confidence of %f", *lag, *coefficient);

#ifdef PLOT
    // Plotting the output with gnuplot
    LOG("Saving plot to '%ld.png'", source_len);
    gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "set term 'png'\n");
    fprintf(gnuplot, "set output 'images/%ld.png'\n", source_len);
    fprintf(gnuplot, "plot '-' with lines title 'sample', '-' with lines"
            " title 'source'\n");
    for (double *i = source_start; i < source_end; i++)
        fprintf(gnuplot, "%f\n", *i);
    fprintf(gnuplot, "e\n");
    for (double *i = sample_start; i < sample_end; i++)
        fprintf(gnuplot, "%f\n", *i);
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
