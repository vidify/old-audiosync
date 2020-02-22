#pragma once

#include <stdlib.h>

// Calculating the Pearson Correlation Coefficient between `source` and
// `sample` between two pointers, applying the formula:
// https://en.wikipedia.org/wiki/Pearson_correlation_coefficient#For_a_sample
//
// This function will only work correctly if end - start != 0.
double pearson_coefficient(double *source_start, const double *source_end,
                           double *sample_start, const double *sample_end);

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
int cross_correlation(double *data1, double *data2, const size_t length,
                      long *displacement, double *coefficient);
