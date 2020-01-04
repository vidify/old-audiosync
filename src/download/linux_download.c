#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <vidify_audiosync/global.h>
#include <vidify_audiosync/ffmpeg_pipe.h>
#include <vidify_audiosync/download/linux_download.h>

#define MAX_LONG_URL 4086
#define MAX_LONG_COMMAND 4086


void *download(void *arg) {
    struct down_data *data = arg;

    char *url;
    url = malloc(sizeof(*url) * MAX_LONG_URL);
    if (url == NULL) {
        perror("audiosync: malloc error");
        goto finish;
    }

    // Obtaining the youtube-dl direct URL to download.
    if (get_audio_url(data->yt_title, &url) < 0) {
        fprintf(stderr, "audiosync: Could not obtain youtube url.\n");
        goto finish;
    }

    // Finally downloading the track data with ffmpeg.
    char *args[] = {
        "ffmpeg", "-y", "-to", MAX_SECONDS_STR, "-i", url, "-ac",
        NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR, "-f", "f64le", "pipe:1", NULL
    };
    read_pipe(data->th_data, args);

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
        fprintf(stderr, "audiosync: Failed to run youtube-dl command\n" );
        goto finish;
    }
    fscanf(fp, "%s", *url);
    pclose(fp);
    fprintf(stderr, "audiosync: obtained youtube-dl URL\n");

    ret = 0;

finish:
    return ret;
}
