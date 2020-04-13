#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <audiosync/audiosync.h>
#include <audiosync/ffmpeg_pipe.h>
#include <audiosync/download/linux_download.h>

#define MAX_LONG_URL 8172
#define MAX_LONG_COMMAND 4086


// Function used for the download thread. In this case, it both obtains the
// direct youtube link to download the audio from, and creates a new
// pulseaudio process to save it inside the thread's data.
//
// In case of error, it will signal the main thread to abort.
void *download(void *arg) {
    struct ffmpeg_data *data = arg;
    LOG("starting download thread");

    // Obtaining the youtube-dl direct URL to download.
    char *url = NULL;
    url = malloc(sizeof(*url) * MAX_LONG_URL);
    if (url == NULL) {
        audiosync_abort();
        perror("url malloc failed");
        goto finish;
    }
    if (get_audio_url(data->title, &url) < 0) {
        audiosync_abort();
        LOG("could not obtain youtube url");
        goto finish;
    }
    LOG("obtained youtube-dl URL for download");

    // Finally downloading the track data with ffmpeg.
    char *args[] = {
        "ffmpeg", "-y", "-to", MAX_SECONDS_STR, "-i", url, "-ac",
        NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR, "-f", "f64le", "pipe:1",
        "-loglevel",
#ifdef NDEBUG
        "fatal",
#else
        "verbose",
#endif
        NULL
    };
    ffmpeg_pipe(data, args);

finish:
    if (url) free(url);
    pthread_exit(NULL);
}

// Obtains the YouTube audio link with Youtube-dl.
//
// Returns 0 on exit, or -1 on error.
int get_audio_url(const char *title, char **url) {
    DEBUG_ASSERT(title); DEBUG_ASSERT(url);
    DEBUG_ASSERT(strlen(title) < MAX_LONG_COMMAND - 1);

    // Creating the full command
    char command[MAX_LONG_COMMAND];
    strcpy(command, "youtube-dl -g -f bestaudio 'ytsearch:");
    strcat(command, title);
    strcat(command, "'");

    // Run the command and read the output
    FILE *fp = popen(command, "r");
    // Failed to run
    if (fp == NULL) {
        return -1;
    }
    fscanf(fp, "%s", *url);
    // Returned an error code
    if (pclose(fp) != 0) {
        return -1;
    }

    return 0;
}
