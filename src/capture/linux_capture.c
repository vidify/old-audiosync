#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include "../global.h"


#define PA_FORMAT PA_SAMPLE_FLOAT32LE
#define BUFSIZE 1024


void *capture(void *arg) {
    struct cap_data *data;
    data = (struct cap_data *) arg;

    int interval_count = 0;
    for (data->len = 0; data->len < data->total_len; ++(data->len)) {
        data->buf[data->len] = 0;

        if (data->len >= data->intervals[interval_count]) {
            pthread_cond_signal(data->done);
            interval_count++;
        }
    }
    pthread_exit(NULL);


    // TODO
    // The sample type to use
    static const pa_sample_spec ss = {
        .format = PA_FORMAT,
        .rate = SAMPLE_RATE,
        .channels = NUM_CHANNELS
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

    size_t step = BUFSIZE * sizeof(float);
    /* int interval_count = 0; */
    for (data->len = 0; data->len < data->total_len; data->len += step) {
        if (pa_simple_read(s, (data->buf) + data->len, BUFSIZE * sizeof(double), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            goto finish;
        }

        // Sending the signal to the main thread when an interval has been
        // completed.
        if (data->len >= data->intervals[interval_count]) {
            pthread_cond_signal(data->done);
            interval_count++;
        }
    }

    ret = 0;


finish:
    if (s)
        pa_simple_free(s);
    
    pthread_exit(NULL);
}
