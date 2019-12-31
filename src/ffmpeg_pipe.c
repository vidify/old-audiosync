#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include "../include/vidify_audiosync/global.h"

#define READ_END 0
#define WRITE_END 1


void read_pipe(struct thread_data *data, char *args[]) {
    int wav_pipe[2];
    if (pipe(wav_pipe) < 0) {
        perror("pipe()");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Child process (ffmpeg), doesn't read the pipe.
        close(wav_pipe[READ_END]);

        // Redirecting stdout to the pipe
        dup2(wav_pipe[WRITE_END], 1);
#ifndef DEBUG
        // Ignoring stderr when debug mode is disabled
        freopen("/dev/null", "w", stderr);
#endif

        execvp("ffmpeg", args);
        printf("ffmpeg command failed.\n");
    } else {
        // Parent process (reading the output pipe), doesn't write.
        close(wav_pipe[WRITE_END]);

        int interval_count = 0;
        data->len = 0;
        int end_thread;
        while (data->len < data->total_len) {
            // Checking if the main process has indicated that this thread
            // should end (accessing it atomically).
            pthread_mutex_lock(data->mutex);
            end_thread = *(data->end);
            pthread_mutex_unlock(data->mutex);
            if (end_thread != 0) {
                kill(pid, SIGKILL);
                break;
            }

            // Reading the data from ffmpeg one by one. If a buffer is used,
            // the read data is incorrect. I should investigate more about this
            // though, as I don't fully understand why this happens.
            if (read(wav_pipe[READ_END], (data->buf + data->len), sizeof(*(data->buf))) < 0) {
                perror("READ");
                break;
            }
            data->len++;

            // Signaling the main thread when a full interval is read.
            if (data->len >= data->intervals[interval_count]) {
                pthread_mutex_lock(data->mutex);
                pthread_cond_signal(data->done);
                pthread_mutex_unlock(data->mutex);
                interval_count++;
            }
        }

        close(wav_pipe[READ_END]);
        wait(NULL);
    }
}

