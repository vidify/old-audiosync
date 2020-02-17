#include <stdio.h>
#include <stdlib.h>
#include <vidify_audiosync/audiosync.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s \"TITLE\" [SINK_NAME]\n", argv[0]);
        exit(1);
    }

    printf("Running audiosync\n");
    int ret;
    long int lag;
    // Running the setup function in case a sink name was provided
    if (argc == 3) {
        printf("Setting up audiosync with sinkname as %s\n", argv[2]);
        audiosync_setup(argv[2]);
    }
    // And calling the main audiosync function.
    ret = audiosync_run(argv[1], &lag);
    printf("Obtained lag (ret=%d): %ld\n", ret, lag);

    return 0;
}
