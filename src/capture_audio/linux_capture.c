#include <stdio.h>
#include <stdlib.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include "linux_capture.h"

#define PA_BUFSIZE 1024
#define PA_FORMAT PA_SAMPLE_FLOAT32LE


int capture_audio(double *data, const int max_samples, int sample_rate,
                  int num_channels) {
    // The sample type to use
    const pa_sample_spec ss = {
        .format = PA_FORMAT,
        .rate = sample_rate,
        .channels = num_channels
    };

    // Saving the return value in case of errors
    int ret = 1;

    // Creating the connection to the server
    int error;
    pa_simple *s = pa_simple_new(NULL,  // Using the default server
                                 "Name",  // The application's name
                                 PA_STREAM_RECORD,  // Recording stream
                                 NULL,  // Using the default device
                                 "Description",  // The stream's description
                                 &ss,  // Sample format declared before
                                 NULL,  // Using the default channel map
                                 NULL,  // Using the default buffering attributes
                                 &error);  // Saving the error code
    if (!s) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }

    /* size_t step = PA_BUFSIZE * sizeof(float); */
    /* for (size_t counter = 0; counter < max_samples; counter += step) { */
        /* if (pa_simple_read(s, data + counter, PA_BUFSIZE * sizeof(data), &error) < 0) { */
            /* fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error)); */
            /* goto finish; */
        /* } */
    /* } */

    ret = 0;


finish:
    if (s)
        pa_simple_free(s);

    return ret;
}
