#ifndef _H_CROSS_CORRELATION
#define _H_CROSS_CORRELATION

#include <stdlib.h>


// Calculating the cross-correlation between two signals `a` and `b`:
//     xcross = ifft(fft(a) * conj(fft(b)))
//
// Both datasets should have the same size. They shouldn't be zero-padded,
// since this function will already do so to length 2N-1. This is needed to
// calculate the circular cross-correlation rather than the regular
// cross-correlation.
//
// Returns the lag in frames the sample has over the source, with a confidence
// between -1 and 1.
//
// In case of error, the function returns -1.
int cross_correlation(double *data1, double *data2, const size_t length,
                      long int *displacement, double *coefficient);


#endif /* _H_CROSS_CORRELATION */
