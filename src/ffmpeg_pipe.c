#define _POSIX_SOURCE  // for kill()
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <audiosync/audiosync.h>

#define PIPE_RD 0
#define PIPE_WR 1
#define BUFSIZE 4096


// Executes the ffmpeg command in the arguments and pipes its data into the
// provided array.
//
// It will send signals to the main thread as the intervals are being
// finished, while also checking the current global status, or updating it in
// case of errors.
//
// Returns -1 in case of error, or zero otherwise.
int ffmpeg_pipe(struct ffmpeg_data *data, char *args[]) {
    DEBUG_ASSERT(args); DEBUG_ASSERT(data); DEBUG_ASSERT(data->title);
    DEBUG_ASSERT(data->buf); DEBUG_ASSERT(data->intervals);
    DEBUG_ASSERT(data->intervals[data->n_intervals-1] == data->total_len);

    int wav_pipe[2];
    int interval_count = 0;
    ssize_t read_bytes;
    pid_t pid;

    if (pipe(wav_pipe) < 0) {
        audiosync_abort();
        perror("audiosync: pipe for wav_pipe failed");
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        audiosync_abort();
        perror("audiosync: fork in read_pipe failed");
        close(wav_pipe[PIPE_RD]); close(wav_pipe[PIPE_WR]);
        return -1;
    }
    if (pid == 0) {
        // Child process (ffmpeg), doesn't read the pipe.
        close(wav_pipe[PIPE_RD]);

        // Redirecting stdout to the pipe
        dup2(wav_pipe[PIPE_WR], 1);
        close(wav_pipe[PIPE_WR]);

        // ffmpeg must be available on the path for exevp to work.
        LOG("running ffmpeg command");
        execvp("ffmpeg", args);

        // If this part of the code is executed, it means that execvp failed.
        audiosync_abort();
        LOG("ffmpeg command failed");
        return -1;
    }

    // Parent process (reading the output pipe), doesn't write.
    close(wav_pipe[PIPE_WR]);
    data->len = 0;
    while (1) {
        // Reading the data from ffmpeg in chunks of size `BUFSIZE`.
        read_bytes = read(wav_pipe[PIPE_RD], data->buf + data->len,
                          BUFSIZE * sizeof(*(data->buf)));

        // Error when trying to read
        if (read_bytes < 0) {
            audiosync_abort();
            perror("audiosync: read for wav_pipe failed");
            close(wav_pipe[PIPE_RD]);
            return -1;
        }

        data->len += read_bytes / sizeof(*(data->buf));

        // End of file or the buffer won't be big enough for the next read.
        if (read_bytes == 0 || data->len + BUFSIZE >= data->total_len) {
            LOG("finished ffmpeg loop");
            break;
        }

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
            LOG("read ABORT_ST, quitting...");
            kill(pid, SIGKILL);
            wait(NULL);
            close(wav_pipe[PIPE_RD]);
            return 0;
        case PAUSED_ST:
            // Suspending the ffmpeg process with a SIGSTOP until the
            // global status is changed from PAUSED_ST.
            LOG("stopping ffmpeg");
            kill(pid, SIGSTOP);

            pthread_mutex_lock(&mutex);
            while (global_status == PAUSED_ST) {
                pthread_cond_wait(&read_continue, &mutex);
            }
            pthread_mutex_unlock(&mutex);

            // After being woken up, checking if the ffmpeg process
            // should continue or stop.
            if (global_status == ABORT_ST) {
                LOG("read ABORT_ST after pause, quitting...");
                kill(pid, SIGKILL);
                wait(NULL);
                close(wav_pipe[PIPE_RD]);
                return 0;
            }

            LOG("resuming ffmpeg");
            kill(pid, SIGCONT);
            break;
        default:
            // RUNNING_ST and IDLE_ST are ignored.
            break;
        }
    }

    // If the track isn't long enough for every interval, the rest of the
    // data is filled with zeroes.
    if (data->len < data->total_len) {
        for (size_t i = data->len; i < data->total_len; i++) {
            data->buf[i] = 0.0;
        }
        data->len = data->total_len;
        // Also sending a signal to the main thread indicating it that all the
        // intervals have been read successfully.
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&interval_done);
        pthread_mutex_unlock(&mutex);
    }

    close(wav_pipe[PIPE_RD]);
    wait(NULL);

    return 0;
}
