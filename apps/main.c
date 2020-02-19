#include <stdio.h>
#include <stdlib.h>
#include <audiosync/audiosync.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s \"TITLE\" [SINK_NAME]\n", argv[0]);
        exit(1);
    }

    int ret;
    long lag;
    // Running the setup function in case a sink name was provided
    if (argc == 3) {
        printf("Setting up audiosync with sinkname %s\n", argv[2]);
        audiosync_setup(argv[2]);
    }
    // And calling the main audiosync function.
    printf("Running audiosync\n");
    ret = audiosync_run(argv[1], &lag);
    printf("Obtained lag (ret=%d): %ld\n", ret, lag);

    return 0;
}
