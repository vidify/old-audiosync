#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <audiosync/audiosync.h>
#include <audiosync/ffmpeg_pipe.h>
#include <audiosync/download/linux_download.h>

#define MAX_LONG_URL 4086
#define MAX_LONG_COMMAND 4086


void *download(void *arg) {
    struct ffmpeg_data *data = arg;

    // Obtaining the youtube-dl direct URL to download.
    char *url = NULL;
    url = malloc(sizeof(*url) * MAX_LONG_URL);
    if (url == NULL) {
        perror("audiosync: url malloc failed");
        goto finish;
    }
    if (get_audio_url(data->title, &url) < 0) {
        log("Could not obtain youtube url");
        goto finish;
    }

    // Finally downloading the track data with ffmpeg.
    char *args[] = {
        "ffmpeg", "-y", "-to", MAX_SECONDS_STR, "-i", url, "-ac",
        NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR, "-f", "f64le", "pipe:1", NULL
    };
    ffmpeg_pipe(data, args);

finish:
    if (url) free(url);
    pthread_exit(NULL);
}


// Obtains the audio direct link with Youtube-dl.
int get_audio_url(char *title, char **url) {
    int ret = -1;

    // Creating the full command
    char command[MAX_LONG_COMMAND];
    strcpy(command, "youtube-dl -g -f bestaudio ytsearch:\"");
    strcat(command, title);
    strcat(command, "\"");

    // Run the command and read the output
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        log("Failed to run youtube-dl command");
        goto finish;
    }
    fscanf(fp, "%s", *url);
    pclose(fp);
    log("obtained youtube-dl URL");

    ret = 0;

finish:
    return ret;
}
