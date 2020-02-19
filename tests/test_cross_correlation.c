#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <audiosync/audiosync.h>
#include <audiosync/cross_correlation.h>


// Testing the cross_correlation function. These results can be compared to
// matlab's implementation:
// https://ch.mathworks.com/help/matlab/ref/xcorr.html
// or with the Python sketch in dev/sketch.py
int main() {
    int ret;
    long int lag;
    size_t length;
    double coef;

    // Both arrays are the same: the displacement should be zero with a
    // coefficient of 1.
    printf(">> Test 1\n");
    double source1[] = { 1.1,2.2,3.3,4.4,5.5,0,0,0,0,0 };
    double sample1[] = { 1.1,2.2,3.3,4.4,5.5 };
    length = sizeof(sample1) / sizeof(*sample1);
    ret = cross_correlation(source1, sample1, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == 0);
    assert(lag == 0);
    assert(coef == 1.0);

    // One array is filled with zeros, so an error should be returned.
    printf(">> Test 2\n");
    double source2[] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
    double sample2[] = { 0,0,0,0,0,0,0 };
    length = sizeof(sample2) / sizeof(*sample2);
    ret = cross_correlation(source2, sample2, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == -1);

    // Both arrays are linearly equal.
    printf(">> Test 3\n");
    double source3[] = { 0,0,0,1,2,3,4,5,6,0,0,0 };
    double sample3[] = { 1,2,3,4,5,6 };
    length = sizeof(sample3) / sizeof(*sample3);
    ret = cross_correlation(source3, sample3, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == 0);
    assert(lag == 3);
    assert(coef > MIN_CONFIDENCE);

    // Similar to the test above, but the other way around.
    printf(">> Test 4\n");
    double source4[] = { 1,2,3,0.4,1.1,0,0,0,0,0,0,0 };
    double sample4[] = { 0,0,0,1,2,3 };
    length = sizeof(sample4) / sizeof(*sample4);
    ret = cross_correlation(source4, sample4, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == 0);
    assert(lag == -3);
    assert(coef > MIN_CONFIDENCE);

    // Other simple tests
    printf(">> Test 5\n");
    double source5[] = { 1,2,3,4,-1.0,0,0,4,3,2,1,0,0,0 };
    double sample5[] = { 0,0,0,1,2,3,4 };
    length = sizeof(sample5) / sizeof(*sample5);
    ret = cross_correlation(source5, sample5, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == 0);
    assert(lag == -3);
    assert(coef > MIN_CONFIDENCE);

    printf(">> Test 6\n");
    double source6[] = { 0,0,0,0,0,1,2,3,4,-1,-3,-5,0,0 };
    double sample6[] = { 1,2,3,4,-1,-3,-5 };
    length = sizeof(sample6) / sizeof(*sample6);
    ret = cross_correlation(source6, sample6, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == 0);
    assert(lag == 5);
    assert(coef > MIN_CONFIDENCE);

    // Using a sine wave with positive linear correlation (same function).
    printf(">> Test 7\n");
    length = 1000;
    double source7[length*2];
    double sample7[length];
    for (size_t i = 0; i < length*2; ++i)
        source7[i] = sin(i);
    for (size_t i = 0; i < length; ++i)
        sample7[i] = sin(i);
    ret = cross_correlation(source7, sample7, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == 0);
    assert(lag == 0);
    assert(coef > MIN_CONFIDENCE);

    // Using a sine wave with negative linear correlation.
    printf(">> Test 8\n");
    length = 1000;
    double source8[length*2];
    double sample8[length];
    for (size_t i = 0; i < length; ++i)
        source8[i] = sin(i + 180);
    for (size_t i = length; i < length*2; ++i)
        source8[i] = 0;
    for (size_t i = 0; i < length; ++i)
        sample8[i] = sin(i);
    ret = cross_correlation(source8, sample8, length, &lag, &coef);
    printf(">> Returned %d: lag=%ld coef=%f\n", ret, lag, coef);
    assert(ret == 0);
    assert(lag == -1);
    assert(coef < -MIN_CONFIDENCE);  // Leaving a margin for precision

    return 0;
}
