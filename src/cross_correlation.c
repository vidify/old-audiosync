#ifdef DEBUG
# define _GNU_SOURCE  // Debug mode uses popen()
# include <time.h>  // Debug mode uses timers
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <complex.h>
#include <fftw3.h>
#include <vidify_audiosync/global.h>



// Data structure used to pass parameters to concurrent FFTW-related functions.
struct fftw_data {
    double *real;
    double complex *cpx;
    size_t length;
    pthread_mutex_t *mutex;
};


// Concurrent implementation of the Fast Fourier Transform using FFTW.
static void *fft(void *thread_arg) {
    // Getting the parameters passed to this thread
    struct fftw_data *data;
    data = (struct fftw_data *) thread_arg;

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
// Both datasets should have the same size. They shouldn't be zero-padded,
// since this function will already do so to length 2N-1. This is needed to
// calculate the circular cross-correlation rather than the regular
// cross-correlation.
//
// Returns the lag in frames the second data set has over the first one, with
// a confidence between -1 and 1.
//
// In case of error, the function returns -1
int cross_correlation(double *input1, double *input2, const size_t input_length,
                      int *displacement, double *coefficient) {
#ifdef DEBUG
    // Benchmarking the matching function
    clock_t start = clock();
#endif
    double *data1, *data2, *results;
    double complex *arr1, *arr2;
    const size_t length = input_length * 2;
    int ret = -1;

    // Zero padding the data.
    data1 = fftw_alloc_real(length);
    memcpy(data1, input1, input_length * sizeof(double));
    memset(data1 + input_length, 0, (length - input_length) * sizeof(double));
    data2 = fftw_alloc_real(length);
    memcpy(data2, input2, input_length * sizeof(double));
    memset(data2 + input_length, 0, (length - input_length) * sizeof(double));

#ifdef DEBUG
    // Plotting the output with gnuplot
    printf(">> Saving initial plot to '%ld_original.png'\n", input_length);
    FILE *gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "set term 'png'\n");
    fprintf(gnuplot, "set output 'images/%ld_original.png'\n", input_length);
    fprintf(gnuplot, "plot '-' with lines title 'data1', '-' with lines title 'data2'\n");
    for (size_t i = 0; i < input_length; ++i)
        fprintf(gnuplot, "%f\n", data1[i]);
    fprintf(gnuplot, "e\n");
    // The second audio file starts at samplesDelay
    for (size_t i = 0; i < input_length; ++i)
        fprintf(gnuplot, "%f\n", data2[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);
#endif

    // Getting the complex results from both FFT. The output length for the
    // complex numbers is n/2+1.
    const size_t cpx_length = (length / 2) + 1;
    arr1 = fftw_alloc_complex(cpx_length);
    arr2 = fftw_alloc_complex(cpx_length);
    results = fftw_alloc_real(length);

    // Initializing the threads and running them
    pthread_mutex_t fft_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t fft1_thread, fft2_thread;
    struct fftw_data fft1_data = {
        .real = data1,
        .cpx = arr1,
        .length = length,
        .mutex = &fft_mutex
    };
    struct fftw_data fft2_data = {
        .real = data2,
        .cpx = arr2,
        .length = length,
        .mutex = &fft_mutex
    };
    if (pthread_create(&fft1_thread, NULL, &fft, (void *) &fft1_data) < 0) {
        perror("pthread_create");
        goto finish;
    }
    if (pthread_create(&fft2_thread, NULL, &fft, (void *) &fft2_data) < 0) {
        perror("pthread_create");
        goto finish;
    }
    if (pthread_join(fft1_thread, NULL) < 0) {
        perror("pthread_join");
        goto finish;
    }
    if (pthread_join(fft2_thread, NULL) < 0) {
        perror("pthread_join");
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
    double abs;
    double max = results[0];
    size_t lag = 0;
    for (size_t i = 1; i < length; ++i) {
        abs = fabs(results[i]);
        if (abs > max) {
            max = abs;
            lag = i;
        }
    }

    // Shifting the first array to the left or to the right by the calculated
    // lag, reusing the previous results array.
    // The zero-padding can be ignored from now on.
    size_t shift_i;
    memcpy(results, data1, sizeof(double) * input_length);
    for (size_t i = 0; i < input_length; ++i) {
        shift_i = (i + lag + input_length) % input_length;
        data1[i] = results[shift_i];
    }

    // Calculating the Pearson Correlation Coefficient:
    // https://en.wikipedia.org/wiki/Pearson_correlation_coefficient#For_a_sample
    // 1. The average for both datasets.
    double sum1 = 0.0;
    double sum2 = 0.0;
    for (size_t i = 0; i < input_length; ++i) {
         sum1 += data1[i];
         sum2 += data2[i];
    }
    double avg1 = sum1 / input_length;
    double avg2 = sum2 / input_length;
    // 2. Applying the definition formula.
    double diff1, diff2;
    double diffprod = 0.0;
    double diff1_squared = 0.0;
    double diff2_squared = 0.0;
    for (size_t i = 0; i < input_length; ++i) {
        diff1 = data1[i] - avg1;
        diff2 = data2[i] - avg2;
        diffprod += diff1 * diff2;
        diff1_squared += diff1 * diff1;
        diff2_squared += diff2 * diff2;
    }
    *coefficient = diffprod / sqrt(diff1_squared * diff2_squared);

    // Checking that the resulting coefficient isn't NaN
    if (*coefficient != *coefficient) goto finish;

    // The returned value should be negative if the displacement is larger
    // than the size of the input array (meaning that the displacement is
    // inverted).
    *displacement = (lag < input_length) ? lag : -(lag % input_length);

#ifdef DEBUG
    printf(">> Result obtained in %f secs\n", (clock() - start) / (double) CLOCKS_PER_SEC);
    printf(">> %d frames of delay with a confidence of %f\n", *displacement, *coefficient);
    // Plotting the output with gnuplot
    printf(">> Saving plot to '%ld.png'\n", input_length);
    gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "set term 'png'\n");
    fprintf(gnuplot, "set output 'images/%ld.png'\n", input_length);
    fprintf(gnuplot, "plot '-' with lines title 'data1', '-' with lines title 'data2'\n");
    for (size_t i = 0; i < input_length; ++i)
        fprintf(gnuplot, "%f\n", data1[i]);
    fprintf(gnuplot, "e\n");
    // The second audio file starts at samplesDelay
    for (size_t i = 0; i < input_length; ++i)
        fprintf(gnuplot, "%f\n", data2[i]);
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    pclose(gnuplot);
#endif

    ret = 0;

finish:
    if (data1) fftw_free(data1);
    if (data2) fftw_free(data2);
    if (arr1) fftw_free(arr1);
    if (arr2) fftw_free(arr2);
    if (results) fftw_free(results);

    return ret;
}
