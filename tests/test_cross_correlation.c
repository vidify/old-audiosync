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
    double arr1[] = { 1,2,3,4,5 };
    double arr2[] = { 1,2,3,4,5 };
    length = sizeof(arr1) / sizeof(*arr1);
    ret = cross_correlation(arr1, arr2, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 0);
    assert(coefficient == 1.0);

    // One array is filled with zeros, so an error should be returned.
    double arr3[] = { 1,2,3,4,5,6,7 };
    double arr4[] = { 0,0,0,0,0,0,0 };
    length = sizeof(arr3) / sizeof(*arr3);
    ret = cross_correlation(arr3, arr4, length, &lag, &coefficient);
    assert(ret == -1);

    // Both arrays are linearly equal.
    double arr5[] = { 1,2,3,4,5,6 };
    double arr6[] = { 3,4,5,6,1,2 };
    length = sizeof(arr5) / sizeof(*arr5);
    ret = cross_correlation(arr5, arr6, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == -2);
    assert(coefficient > MIN_CONFIDENCE);

    // Similar to the test above, but the other way around.
    double arr7[] = { 4,5,6,1,2,3 };
    double arr8[] = { 1,2,3,4,5,6 };
    length = sizeof(arr7) / sizeof(*arr7);
    ret = cross_correlation(arr7, arr8, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 3);
    assert(coefficient > MIN_CONFIDENCE);

    // Other simple tests
    double arr9[] = { 1,2,3,4,0,0,0 };
    double arr10[] = { 3,4,0,0,0,1,2 };
    length = sizeof(arr9) / sizeof(*arr9);
    ret = cross_correlation(arr9, arr10, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == -2);
    assert(coefficient > MIN_CONFIDENCE);

    double arr11[] = { 3,4,0,0,0,1,2 };
    double arr12[] = { 1,2,3,4,0,0,0 };
    length = sizeof(arr11) / sizeof(*arr11);
    ret = cross_correlation(arr11, arr12, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 2);
    assert(coefficient > MIN_CONFIDENCE);

    // Using a sine wave with positive linear correlation (same function).
    length = 1000;
    double arr13[length];
    double arr14[length];
    for (size_t i = 0; i < length; ++i) {
        arr13[i] = arr14[i] = sin(i);
    }
    ret = cross_correlation(arr13, arr14, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 0);
    assert(coefficient > MIN_CONFIDENCE);

    // Using a sine wave with negative linear correlation.
    double arr15[length];
    double arr16[length];
    for (size_t i = 0; i < length; ++i) {
        arr15[i] = sin(i);
        arr16[i] = sin(i + 180);
    }
    ret = cross_correlation(arr15, arr16, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == -1);
    assert(coefficient < -MIN_CONFIDENCE);  // Leaving a margin for precision

    return 0;
}
