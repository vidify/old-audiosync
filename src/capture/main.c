// Spotivids extension to record desktop audio.
// 
// The audio has very specific properties (mono, 48000KHz) so that it can
// be processed later without trouble. This module itself simply uses
// PortAudio to record the audio. It uses PortAudio specifically because
// it's intended to be cross-platform. Most of this program's content
// is taken from a PortAudio example.


#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 1
#define DITHER_FLAG 0
#define WRITE_TO_FILE 0
#define PA_SAMPLE_TYPE paFloat32
#define SAMPLE_SILENCE 0.0f
#define PRINTF_S_FORMAT "%.8f"

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ms>\n", argv[0]);
        exit(1);
    }
    const int ms = atoi(argv[1]);
    const int maxSamples = (ms / 1000) * SAMPLE_RATE;

    // The recorded data will be saved into an array rather than a file.
    double *data = malloc(maxSamples * sizeof(double));
    if (data == NULL) {
        fprintf(stderr, "Allocation of capture data failed.\n");
        exit(1);
    }

    // Cross-platform implementation with support for Linux and Windows for
    // now. Linux uses PulseAudio.
    int ret = 0;
#if defined(__linux__)
#include "linux_capture.h"
    ret = captureAudio(maxSamples, data);
#elif defined(_WIN32)
#include "windows_capture.h"
    ret = captureAudio(maxSamples, data);
#else
#error "Platform not supported."
#endif

    FILE *gnuplot = popen("gnuplot", "w");
    fprintf(gnuplot, "plot '-'\n");
    for (int i = 0; i < maxSamples; ++i) {
        fprintf(gnuplot, "%f\n", data[i]);
    }
    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    printf("Press enter to continue\n");
    getchar();
    pclose(gnuplot);

    return ret;
}
