#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vidify_audiosync/global.h>
#include <vidify_audiosync/ffmpeg_pipe.h>
#include <vidify_audiosync/capture/linux_capture.h>


// NOTE: for now requires to change the captured sink in pavucontrol to
// 'Monitor of ...'. Otherwise the results will be empty.
void *capture(void *arg) {
    struct thread_data *data;
    data = (struct thread_data *) arg;

    char *args[] = {
        "ffmpeg", "-y", "-to", "15", "-f", "pulse", "-i", "default",
        "-ac", NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR, "-f", "f64le",
        "pipe:1", NULL
    };
    read_pipe(data, args);

    pthread_exit(NULL);
}
