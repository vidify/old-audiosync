#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <vidify_audiosync/audiosync.h>
#include <vidify_audiosync/cross_correlation.h>


// Testing the Pearson Correlation Coefficient. These results can be compared
// to Python's scipy.stats.pearsonr and numpy.corrcoef.
// test_cross_correlation already tests this function, since it's called
// inside cross_correlation, so this file only contains basic checks.
int main() {
    double ret;
    size_t len;

    // Basic tests for identical source and sample.
    printf(">> Test 1\n");
    double source1[] = { 1.0,2.1,3.2,4.3,5.4,6.5,7.6,8.7,9.8,10.9 };
    double sample1[] = { 1.0,2.1,3.2,4.3,5.4,6.5,7.6,8.7,9.8,10.9 };
    len = sizeof(source1) / sizeof(*source1);
    ret = pearson_coefficient(source1, sample1, 0, len);
    printf("Returned %f between %ld and %ld\n", ret, 0L, len);
    assert(ret == 1.0);
    ret = pearson_coefficient(source1, sample1, 0, 5);
    printf("Returned %f between %ld and %ld\n", ret, 0L, 5L);
    assert(ret == 1.0);
    ret = pearson_coefficient(source1, sample1, 5, len);
    printf("Returned %f between %ld and %ld\n", ret, 5L, len);
    assert(ret == 1.0);

    // Testing negative linear correlation
    printf(">> Test 2\n");
    double source2[] = { 1,2,3,4 };
    double sample2[] = { 4,3,2,1 };
    len = sizeof(source2) / sizeof(*source2);
    ret = pearson_coefficient(source2, sample2, 0, len);
    printf("Returned %f between %ld and %ld\n", ret, 0L, len);
    assert(ret == -1.0);

    // Testing that on error, NaN is returned (in this case, an array filled
    // with zeroes isn't defined in the Pearson Correlation Coefficient).
    printf(">> Test 3\n");
    double source3[] = { 1,2,3,4 };
    double sample3[] = { 0,0,0,0 };
    len = sizeof(source3) / sizeof(*source3);
    ret = pearson_coefficient(source3, sample3, 0, len);
    printf("Returned %f between %ld and %ld\n", ret, 0L, len);
    assert(ret != ret);

    return 0;
}
