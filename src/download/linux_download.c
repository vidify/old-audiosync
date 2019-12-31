#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../include/vidify_audiosync/global.h"
#include "../../include/vidify_audiosync/ffmpeg_pipe.h"


void *download(void *arg) {
    struct down_data *data;
    data = (struct down_data *) arg;

    char *args[] = {
        "ffmpeg", "-y", "-to", "15", "-i", data->url, "-ac",
        NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR, "-f", "f64le", "pipe:1",
        NULL
    };
    read_pipe(data->th_data, args);

    pthread_exit(NULL);
}
