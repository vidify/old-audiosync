#define _POSIX_SOURCE  // kill() isn't technically a Linux function
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vidify_audiosync/audiosync.h>

#define READ_END 0
#define WRITE_END 1


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
        close(wav_pipe[READ_END]);

        // Redirecting stdout to the pipe
        dup2(wav_pipe[WRITE_END], 1);

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
    } else {
        // Parent process (reading the output pipe), doesn't write.
        close(wav_pipe[WRITE_END]);

        int interval_count = 0;
        data->len = 0;
        while (data->len < data->total_len) {
            // Checking if the main process has indicated that this thread
            // should end (accessing it atomically).
            switch (audiosync_status()) {
            case ABORT_ST:
                fprintf(stderr, "audiosync: read ABORT_ST, quitting...\n");
                kill(pid, SIGKILL);
                wait(NULL);
                goto finish;
            case PAUSED_ST:
                // Pausing the ffmpeg process with a SIGSTOP until the
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

            // Reading the data from ffmpeg one by one. If a buffer is used,
            // the read data is incorrect.
            if (read(wav_pipe[READ_END], (data->buf + data->len),
                     sizeof(*(data->buf))) < 0) {
                perror("audiosync: read for wav_pipe failed");
                audiosync_abort();
                goto finish;
            }
            data->len++;

            // Signaling the main thread when a full interval is read.
            if (data->len >= data->intervals[interval_count]) {
                pthread_mutex_lock(&mutex);
                pthread_cond_signal(&interval_done);
                pthread_mutex_unlock(&mutex);
                ++interval_count;
            }
        }

        close(wav_pipe[READ_END]);
        wait(NULL);
    }

    ret = 0;

finish:
    return ret;
}

