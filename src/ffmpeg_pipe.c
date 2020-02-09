#define _POSIX_SOURCE  // for kill()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vidify_audiosync/audiosync.h>

#define PIPE_RD 0
#define PIPE_WR 1
#define BUFSIZE 4096


// Executes the ffmpeg command in the arguments and pipes its data into the
// provided array.
int ffmpeg_pipe(struct ffmpeg_data *data, char *args[]) {
    int ret = -1;
    int wav_pipe[2];
    if (pipe(wav_pipe) < 0) {
        perror("audiosync: pipe for wav_pipe failed");
        audiosync_abort();
        goto finish;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("audiosync: fork in read_pipe failed");
        audiosync_abort();
        goto finish;
    } else if (pid == 0) {
        // Child process (ffmpeg), doesn't read the pipe.
        close(wav_pipe[PIPE_RD]);

        // Redirecting stdout to the pipe
        dup2(wav_pipe[PIPE_WR], 1);
        close(wav_pipe[PIPE_WR]);

#ifndef DEBUG
        // Ignoring stderr when debug mode is disabled
        freopen("/dev/null", "w", stderr);
#endif

        // ffmpeg must be available on the path for exevp to work.
        fprintf(stderr, "audiosync: running ffmpeg command\n");
        execvp("ffmpeg", args);

        // If this part of the code is executed, it means that execvp failed.
        fprintf(stderr, "audiosync: ffmpeg command failed\n");
        audiosync_abort();
        goto finish;
    }

    // Parent process (reading the output pipe), doesn't write.
    close(wav_pipe[PIPE_WR]);

    int interval_count = 0;
    ssize_t read_bytes;
    data->len = 0;
    while (1) {
        // Reading the data from ffmpeg in chunks of size `BUFSIZE`.
        read_bytes = read(wav_pipe[PIPE_RD], (data->buf + data->len),
                          BUFSIZE * sizeof(*(data->buf)));
        if (read_bytes == 0 || data->len + BUFSIZE >= data->total_len) {
            // End of file or the buffer won't be big enough for the next
            // read.
            fprintf(stderr, "audiosync: finished ffmpeg loop\n");
            break;
        } else if (read_bytes < 0) {
            // Error
            perror("audiosync: read for wav_pipe failed");
            audiosync_abort();
            goto finish;
        }

        data->len += read_bytes / sizeof(*(data->buf));

        // Signaling the main thread when a full interval is read.
        if (data->len >= data->intervals[interval_count]) {
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&interval_done);
            pthread_mutex_unlock(&mutex);
            interval_count++;
        }

        // Checking if the main process has indicated that this thread
        // should end (accessing it atomically).
        switch (audiosync_status()) {
        case ABORT_ST:
            fprintf(stderr, "audiosync: read ABORT_ST, quitting...\n");
            kill(pid, SIGKILL);
            wait(NULL);
            goto finish;
        case PAUSED_ST:
            // Suspending the ffmpeg process with a SIGSTOP until the
            // global status is changed from PAUSED_ST.
            fprintf(stderr, "audiosync: stopping ffmpeg\n");
            kill(pid, SIGSTOP);

            pthread_mutex_lock(&mutex);
            while (global_status == PAUSED_ST) {
                pthread_cond_wait(&ffmpeg_continue, &mutex);
            }
            pthread_mutex_unlock(&mutex);

            // After being woken up, checking if the ffmpeg process
            // should continue or stop.
            if (global_status == ABORT_ST) {
                fprintf(stderr, "audiosync: read ABORT_ST after pause,"
                        " quitting...\n");
                kill(pid, SIGKILL);
                wait(NULL);
            } else {
                fprintf(stderr, "audiosync: resuming ffmpeg\n");
                kill(pid, SIGCONT);
            }

            break;
        default:
            // RUNNING_ST and IDLE_ST are ignored.
            break;
        }
    }

    // If the track isn't long enough for every interval, the rest of the
    // data is filled with zeroes.
    if (data->len < data->total_len) {
        memset(data->buf + data->len, 0.0,
               sizeof(*(data->buf)) * (data->total_len - data->len));
        data->len = data->total_len;
        // Also sending a signal to the main thread indicating it that all the
        // intervals have been read successfully.
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&interval_done);
        pthread_mutex_unlock(&mutex);
    }

    close(wav_pipe[PIPE_RD]);
    wait(NULL);

    ret = 0;

finish:
    return ret;
}

