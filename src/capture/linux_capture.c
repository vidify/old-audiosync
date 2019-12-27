#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include "../global.h"

#define READ_END 0
#define WRITE_END 1


// NOTE: for now requires to change the captured sink in pulseaudio to
// 'Monitor of ...'. Otherwise the results will be empty.
void *capture(void *arg) {
    struct cap_data *data;
    data = (struct cap_data *) arg;
    
    int wav_pipe[2];
    if (pipe(wav_pipe) < 0) {
        perror("pipe()");
        goto finish;
    }

    pid_t pid = fork();
    if (pid < 0) {
        // fork() failed
        perror("fork()");
        goto finish;
    } else if (pid == 0) {
        // Child process (ffmpeg)
        close(wav_pipe[READ_END]);  // Child won't read the pipe
        // ffmpeg $FFMPEG_FLAGS -f pulse -i "$defaultSink" "$1"
        // The order of the arguments is important
        char *args[] = {
            "ffmpeg", "-y", "-to", "15", "-f", "pulse", "-i", "default",
            "-ac", NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR, "-f", "f64le",
            "pipe:1", NULL
        };
        dup2(wav_pipe[WRITE_END], 1);  // Redirecting stdout to the pipe
#ifndef DEBUG
        freopen("/dev/null", "w", stderr);  // Ignoring stderr
#endif
        printf("Starting capture\n");
        execvp("ffmpeg", args);  // Executing the command
        printf("ffmpeg command failed.\n");
    } else {
        // Parent process (reading the output pipe)
        close(wav_pipe[WRITE_END]);  // Parent won't write the pipe

        // Reading pipe
        int interval_count = 0;
        data->len = 0;
        while (data->len < data->total_len) {
            // Checking if the main process has indicated that this thread
            // should end.
            if (*(data->end) != 0) {
                kill(pid, SIGKILL);
                break;
            }

            // TODO read bigger chunks
            if (read(wav_pipe[READ_END], (data->buf + data->len), sizeof(double)) < 0) {
                break;
            }

            // Signaling the main thread when a full interval is read.
            if (data->len >= data->intervals[interval_count]) {
                pthread_cond_signal(data->done);
                interval_count++;
            }

            ++(data->len);
        }

        close(wav_pipe[READ_END]);
        wait(NULL);
    }

finish:
    pthread_exit(NULL);
}
