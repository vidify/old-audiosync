#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <vidify_audiosync/cross_correlation.h>
#include <vidify_audiosync/global.h>


// Testing the cross_correlation function. These results can be compared to
// matlab's implementation:
// https://ch.mathworks.com/help/matlab/ref/xcorr.html
// or with the Python sketch in dev/sketch.py
int main() {
    int ret;
    long int lag;
    size_t length;
    double coefficient;

    // Both arrays are the same: the displacement should be zero with a
    // confidence of 1.
    double source1[] = { 1,2,3,4,5 };
    double sample1[] = { 1,2,3,4,5 };
    length = sizeof(source1) / sizeof(*source1);
    ret = cross_correlation(source1, sample1, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 0);
    assert(coefficient == 1.0);

    // One array is filled with zeros, so an error should be returned.
    double source2[] = { 1,2,3,4,5,6,7 };
    double sample2[] = { 0,0,0,0,0,0,0 };
    length = sizeof(source2) / sizeof(*source2);
    ret = cross_correlation(source2, sample2, length, &lag, &coefficient);
    assert(ret == -1);

    // Both arrays are linearly equal.
    double source3[] = { 1,2,3,4,5,6 };
    double sample3[] = { 3,4,5,6,1,2 };
    length = sizeof(source3) / sizeof(*source3);
    ret = cross_correlation(source3, sample3, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 2);
    assert(coefficient > MIN_CONFIDENCE);

    // Similar to the test above, but the other way around.
    double source4[] = { 4,5,6,1,2,3 };
    double sample4[] = { 1,2,3,4,5,6 };
    length = sizeof(source4) / sizeof(*source4);
    ret = cross_correlation(source4, sample4, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == -3);
    assert(coefficient > MIN_CONFIDENCE);

    // Other simple tests
    double source5[] = { 1,2,3,4,0,0,0 };
    double sample5[] = { 3,4,0,0,0,1,2 };
    length = sizeof(source5) / sizeof(*source5);
    ret = cross_correlation(source5, sample5, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 2);
    assert(coefficient > MIN_CONFIDENCE);

    double source6[] = { 3,4,0,0,0,1,2 };
    double sample6[] = { 1,2,3,4,0,0,0 };
    length = sizeof(source6) / sizeof(*source6);
    ret = cross_correlation(source6, sample6, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == -2);
    assert(coefficient > MIN_CONFIDENCE);

    // Using a sine wave with positive linear correlation (same function).
    length = 1000;
    double source7[length];
    double sample7[length];
    for (size_t i = 0; i < length; ++i) {
        source7[i] = sample7[i] = sin(i);
    }
    ret = cross_correlation(source7, sample7, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 0);
    assert(coefficient > MIN_CONFIDENCE);

    // Using a sine wave with negative linear correlation.
    double source8[length];
    double sample8[length];
    for (size_t i = 0; i < length; ++i) {
        source8[i] = sin(i);
        sample8[i] = sin(i + 180);
    }
    ret = cross_correlation(source8, sample8, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 1);
    assert(coefficient < -MIN_CONFIDENCE);  // Leaving a margin for precision

    return 0;
}
