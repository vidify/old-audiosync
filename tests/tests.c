#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../src/cross_correlation.h"


int main() {
    int ret;

    // Both arrays are the same: the displacement should be zero with a
    // confidence of 1.
    double arr1[] = { 1,2,3,4,5 };
    double arr2[] = { 1,2,3,4,5 };
    size_t length = sizeof(arr1) / sizeof(*arr1);
    int lag;
    double coefficient;
    ret = cross_correlation(arr1, arr2, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 0);
    assert(coefficient == 1.0);

    // Both arrays are linearly equal.
    double arr3[] = { 1,2,3,4,5,6 };
    double arr4[] = { 4,5,6,1,2,3 };
    length = sizeof(arr3) / sizeof(*arr3);
    ret = cross_correlation(arr3, arr4, length, &lag, &coefficient);
    assert(ret == 0);
    assert(lag == 3);
    assert(coefficient == 1.0);

    // One array is filled with zeros, so an error should be returned.
    double arr5[] = { 1,2,3,4,5,6,7 };
    double arr6[] = { 0,0,0,0,0,0,0 };
    length = sizeof(arr5) / sizeof(*arr5);
    ret = cross_correlation(arr5, arr6, length, &lag, &coefficient);
    assert(ret == -1);

    return 0;
}
