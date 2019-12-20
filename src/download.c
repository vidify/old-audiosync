#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>


int download(char *url, double *data, size_t length, char *sample_rate,
             char *num_channels) {
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
        char *args[] = {"ffmpeg", "-y", "-ac", num_channels, "-r", sample_rate,
                        "-to", "15", "-i", url, "-f", "s16le", "pipe:1", NULL};
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
        while (read(wav_pipe[0], buf, sizeof(int16_t)) > 0 && i < length) {
            data[i] = buf[0];
            printf("Output: %d %d\n", i+1, buf[0]);
            ++i;
        }
        /* sleep(10); */
        close(wav_pipe[0]);

        wait(NULL);
    }
 
    return 0;
}
