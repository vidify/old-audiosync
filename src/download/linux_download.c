#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../global.h"

#define READ_END 0
#define WRITE_END 1


void *download(void *arg) {
    struct down_data *data;
    data = (struct down_data *) arg;

    int wav_pipe[2];
    if (pipe(wav_pipe) < 0) {
        perror("pipe()");
        goto fail;
    }

    pid_t pid = fork();
    if (pid < 0) {
        // fork() failed
        perror("fork()");
        goto fail;
    } else if (pid == 0) {
        // Child process (ffmpeg)
        close(wav_pipe[READ_END]);  // Child won't read the pipe

        char *args[] = {"ffmpeg", "-y", "-ac", NUM_CHANNELS_STR, "-r",
                        SAMPLE_RATE_STR, "-to", "15", "-i", data->url, "-f",
                        "f64le", "pipe:1", NULL};
        dup2(wav_pipe[WRITE_END], 1);  // Redirecting stdout to the pipe
#ifndef DEBUG
        freopen("/dev/null", "w", stderr);  // Ignoring stderr
#endif
        printf("Starting download\n");
        execvp("ffmpeg", args);  // Executing the command
        printf("ffmpeg command failed.\n");
    } else {
        // Parent process (reading the output pipe)
        close(wav_pipe[WRITE_END]);  // Parent won't write the pipe

        // Reading pipe
        int interval_count = 0;
        for (data->len = 0; data->len < data->total_len; ++(data->len)) {
            // TODO read bigger chunks
            if (read(wav_pipe[READ_END], (data->buf + data->len), sizeof(double)) < 0) {
                break;
            }

            if (data->len >= data->intervals[interval_count]) {
                pthread_cond_signal(data->done);
                interval_count++;
            }
        }

        close(wav_pipe[READ_END]);
        wait(NULL);
        printf("Finished download pipe. Read %ld bytes\n", data->len);

    }

    pthread_exit(NULL);

fail:

    pthread_exit(NULL);
}
