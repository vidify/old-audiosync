#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../global.h"


void *download(void *arg) {
    struct down_data *data;
    data = (struct down_data *) arg;

    int interval_count = 0;
    for (data->len = 0; data->len < data->total_len; ++(data->len)) {
        data->buf[data->len] = 0;

        if (data->len >= data->intervals[interval_count]) {
            pthread_cond_signal(data->done);
            interval_count++;
        }
    }
    pthread_exit(NULL);


    // TODO
    int wav_pipe[2];
    if (pipe(wav_pipe) < 0) {
        perror("pipe()");
        exit(1);
    }

    pid_t pid;
    pid = fork();
    if (pid < 0) {
        // fork() failed
        perror("fork()");
        exit(1);
    } else if (pid == 0) {
        // Child process (ffmpeg)
        char *args[] = {"ffmpeg", "-y", "-ac", NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR,
                        "-to", "15", "-i", data->url, "-f", "s16le", "pipe:1", NULL};
        close(wav_pipe[0]);  // Child won't read the pipe
        dup2(wav_pipe[1], 1);  // Redirecting stdout to the pipe
        execvp("ffmpeg", args);  // Executing the command
        /* close(wav_pipe[1]); */
        printf("ffmpeg command failed.\n");
    } else {
        // Parent process (reading the output pipe)
        close(wav_pipe[1]);  // Parent won't write the pipe
        int16_t buf[1];
        // Reading pipe
        int i = 0;
        /* int interval_count = 0; */
        while (read(wav_pipe[0], buf, sizeof(int16_t)) > 0 && i < data->total_len) {
            data->buf[i] = buf[0];
            printf("Output: %d %d\n", i+1, buf[0]);
            ++i;
            // Sending the signal to the main thread when an interval has been
            // completed.
            if (data->len >= data->intervals[interval_count]) {
                pthread_cond_signal(data->done);
                interval_count++;
            }
        }
        /* sleep(10); */
        close(wav_pipe[0]);

        wait(NULL);
    }
}
