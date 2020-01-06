#include <stdio.h>
#include <stdlib.h>
#include <vidify_audiosync/audiosync.h>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s \"TITLE\"\n", argv[0]);
        exit(1);
    }

    printf("Running audiosync\n");
    int ret;
    long int lag;
    ret = audiosync_run(argv[1], &lag);
    printf("Obtained lag (ret=%d): %ld\n", ret, lag);

    return 0;
}
