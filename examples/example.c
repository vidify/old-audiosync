#include <stdio.h>
#include <stdlib.h>
#include "../src/audiosync.h"


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s URL\n", argv[0]);
        exit(1);
    }

    int ret = get_lag(argv[1]);
    printf("Returned value: %d\n", ret);

    return 0;
}
