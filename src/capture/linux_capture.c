#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vidify_audiosync/audiosync.h>
#include <vidify_audiosync/ffmpeg_pipe.h>
#include <vidify_audiosync/capture/linux_capture.h>


void *capture(void *arg) {
    struct ffmpeg_data *data = arg;

    // Finally starting to record the audio with ffmpeg.
    char *args[] = {
        "ffmpeg", "-y", "-to", MAX_SECONDS_STR, "-f", "pulse", "-i",
        "audiosync.monitor", "-ac", NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR,
        "-f", "f64le", "pipe:1", NULL
    };
    ffmpeg_pipe(data, args);

    pthread_exit(NULL);
}
