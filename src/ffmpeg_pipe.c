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
        perror("audiosync: pipe for wav_pipe error");
        audiosync_abort();
        goto finish;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("audiosync: fork in read_pipe error");
        audiosync_abort();
        goto finish;
    } else if (pid == 0) {
        // Child process (ffmpeg), doesn't read the pipe.
        close(wav_pipe[READ_END]);

        // Redirecting stdout to the pipe
        dup2(wav_pipe[WRITE_END], 1);

        // ffmpeg must be available on the path for exevp to work.
        execvp("ffmpeg", args);

        // If this part of the code is executed, it means that execvp failed.
        fprintf(stderr, "audiosync: ffmpeg command failed.\n");
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
                kill(pid, SIGKILL);
                goto finish;
            case PAUSED_ST:
                // Pausing the ffmpeg process with a SIGSTOP until the
                // global status is changed from PAUSED_ST.
                kill(pid, SIGSTOP);

                global_status_t s;
                while ((s = audiosync_status()) == PAUSED_ST) {
                    pthread_cond_wait(&ffmpeg_continue, &mutex);
                }

                // After being woken up, checking if the ffmpeg process
                // should continue or stop.
                if (s == ABORT_ST) {
                    kill(pid, SIGKILL);
                } else {
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
                perror("audiosync: read for wav_pipe");
                audiosync_abort();
                goto finish;
            }
            data->len++;

            // Signaling the main thread when a full interval is read.
            if (data->len >= data->intervals[interval_count]) {
                pthread_mutex_lock(&mutex);
                pthread_cond_signal(&interval_done);
                pthread_mutex_unlock(&mutex);
                interval_count++;
            }
        }

        close(wav_pipe[READ_END]);
        wait(NULL);
    }

    ret = 0;

finish:
    return ret;
}

