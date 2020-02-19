#include <stdio.h>
#include <assert.h>
#include <vidify_audiosync/audiosync.h>
#include <vidify_audiosync/capture/linux_capture.h>


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s \"SINKNAME\"\n", argv[0]);
        return 1;
    }

    int ret = pulseaudio_setup(argv[1]);

    return ret == 0 ? 0 : 1;
}
