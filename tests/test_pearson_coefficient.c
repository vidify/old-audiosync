#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <audiosync/audiosync.h>
#include <audiosync/cross_correlation.h>


// Testing the Pearson Correlation Coefficient. These results can be compared
// to Python's scipy.stats.pearsonr and numpy.corrcoef.
// test_cross_correlation already tests this function, since it's called
// inside cross_correlation, so this file only contains basic checks.
int main() {
    double ret;
    size_t len;
    size_t lag;

    // Basic tests for identical source and sample.
    // Test 1 simulates a displacement to the left.
    printf(">> Test 1\n");
    double source1[] = { 1.0,2.1,3.2,4.3,5.4,6.5,7.6,8.7,9.8,10.9 };
    double sample1[] = { 0,0,1.0,2.1,3.2,4.3,5.4,6.5,7.6,8.7 };
    len = sizeof(sample1) / sizeof(*sample1);
    lag = -2;
    ret = pearson_coefficient(source1, source1 + lag + len,
                              sample1 - lag, sample1 + len);
    printf(">> Returned %f\n", ret);
    assert(ret == 1.0);

    // Test 2 simulates a displacement to the right.
    printf(">> Test 2\n");
    double source2[] = { 0,0,0,0,100,200,300,400,500,600,700 };
    double sample2[] = { 100,200,300,400,500 };
    len = sizeof(sample2) / sizeof(*sample2);
    lag = 4;
    ret = pearson_coefficient(source2 + lag, source2 + lag + len,
                              sample2, sample2 + len);
    printf(">> Returned %f\n", ret);
    assert(ret == 1.0);

    // Testing negative linear correlation
    printf(">> Test 3\n");
    double source3[] = { 1,2,3,4 };
    double sample3[] = { 4,3,2,1 };
    len = sizeof(source3) / sizeof(*source3);
    ret = pearson_coefficient(source3, source3 + len, sample3, sample3 + len);
    printf(">> Returned %f between %ld and %ld\n", ret, 0L, len);
    assert(ret == -1.0);

    // Testing that on error, NaN is returned (in this case, an array filled
    // with zeroes isn't defined in the Pearson Correlation Coefficient).
    printf(">> Test 4\n");
    double source4[] = { 1,2,3,4 };
    double sample4[] = { 0,0,0,0 };
    len = sizeof(source4) / sizeof(*source4);
    ret = pearson_coefficient(source4, source4 + len, sample4, sample4 + len);
    printf(">> Returned %f between %ld and %ld\n", ret, 0L, len);
    assert(ret != ret);

    return 0;
}
