#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <pulse/pulseaudio.h>
#include <vidify_audiosync/audiosync.h>
#include <vidify_audiosync/ffmpeg_pipe.h>
#include <vidify_audiosync/capture/linux_capture.h>

#define SINK_NAME "audiosync"
#define MAX_NUM_SINKS 16
#define MAX_LONG_NAME 512
#define MAX_LONG_DESCRIPTION 256


// The possible states when connecting to the PulseAudio server.
typedef enum {
    PA_REQUEST_NOT_READY = 0,
    PA_REQUEST_READY = 1,
    PA_REQUEST_ERROR = 2
} pa_request_state_t;

// The stream name is a global so that it can be accessed inside callback
// functions.
static char *stream_name;
// Variable that acts as a boolean to indicate if the setup function was
// called and worked successfully.
static int use_default = 1;


// This PulseAudio function acts as a callback when the context changes state.
// We really only care about when it's ready or if it has failed.
static void pa_state_cb(pa_context *c, void *userdata) {
	int *server_status = userdata;

	pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
        *server_status = PA_REQUEST_ERROR;
    } else if (state == PA_CONTEXT_READY) {
        *server_status = PA_REQUEST_READY;
    }
}

// Simple callback function called after the module has been loaded with
// pa_module_load. It returns the operation index, which can be an error.
static void load_module_cb(pa_context *c, uint32_t index, void *userdata) {
    int *ret = userdata;

    *ret = (index == PA_INVALID_INDEX) ? -1 : 0;
}

// Simple callback function that returns if the operation was successful.
static void operation_cb(pa_context *c, int success, void *userdata) {
    int *ret = userdata;
    *ret = success;
}

// Callback used to look for the media player's stream to obtain its index.
static void find_stream_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata) {
    uint32_t *index = userdata;

    // If eol is positive it means there aren't more available streams.
    // If the stream wasn't found, the index value will remain as its initial
    // value, which should be PA_INVALID_INDEX to know if this procedure
    // worked.
    if (eol > 0) {
        return;
    }

    // Tries to find the stream in its application.name property. If any
    // media players didn't set this, other properties like media.name or
    // application.process.binary could be checked too.
    const char *name = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME);
    // Some streams don't have the required property.
    if (name == NULL) {
        return;
    }
    if (strstr(name, stream_name) != NULL) {
        *index = i->index;
    }
}

// This function will run the pulseaudio mainloop with everything that needs
// to be done:
//     1. Create a null sink for the audiosync extension
//     2. Loopback into the new sink
//     3. Search the media player's stream
//     4. Move the found stream into the new sink
//
// Pulseaudio asynchronously handles all the requests, so this mainloop
// structure is needed to handle them one by one.
int pulseaudio_setup(char *name) {
    // Saving the stream name as a global variable so that it can be accessed
    // within the callback functions.
    stream_name = strdup(name);

    // Define the pulseaudio loop and connection variables
    pa_mainloop *pa_ml = NULL;
    pa_mainloop_api *pa_mlapi = NULL;
    pa_operation *pa_op = NULL;
    pa_context *pa_ctx = NULL;

    // Create a mainloop API and connection to the default server
    pa_ml = pa_mainloop_new();
    pa_mlapi = pa_mainloop_get_api(pa_ml);
    pa_ctx = pa_context_new(pa_mlapi, SINK_NAME);
    pa_context_connect(pa_ctx, NULL, 0, NULL);

    // This function defines a callback so the server will tell us its state.
    // Our callback will wait for the state to be ready. The callback will
    // modify the variable so we know when we have a connection and it's
    // ready or when it has failed.
    pa_request_state_t server_status = PA_REQUEST_NOT_READY;
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &server_status);

    // To keep track of our requests
    int state = 0;
    // Variable assigned after the music player's stream index has been found
    // so that it can be moved to the new sink.
    uint32_t stream_index = PA_INVALID_INDEX;
    // This function's return value.
    int ret = -1;
    while (1) {
        // Checking the status of the PulseAudio server before sending any
        // requests.
        if (server_status == PA_REQUEST_NOT_READY) {
            // We can't do anything until PA is ready, so just iterate the
            // mainloop and continue.
            pa_mainloop_iterate(pa_ml, 1, NULL);
            continue;
        } else if (server_status == PA_REQUEST_ERROR) {
            // Error connecting to the server.
            fprintf(stderr, "Error when connecting to the server.\n");
            ret = -1;
            goto error;
        }

        // At this point, we're connected to the server and ready to make
        // requests.
        switch (state) {
        case 0:
            // This sends an operation to the server. The first one will be
            // loading a new sink for audiosync.
            pa_op = pa_context_load_module(
                pa_ctx, "module-null-sink", "sink_name=" SINK_NAME
                " sink_properties=device.description=" SINK_NAME
                " format=float32le",
                load_module_cb, &ret);

            state++;
            break;

        case 1:
            // Now we wait for our operation to complete. When it's
            // complete, we move along to the next state, which is loading
            // the loopback module for the new virtual sink created.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                // Checking the returned value by the previous state's module
                // load.
                if (ret < 0) {
                    fprintf(stderr, "module-null-sink failed to load\n");
                    goto error;
                }

                // latency_msec is needed so that the virtual sink's audio is
                // synchronized with the desktop audio (disables the delay).
                pa_op = pa_context_load_module(
                    pa_ctx, "module-loopback", "source=" SINK_NAME ".monitor"
                    " latency_msec=1", load_module_cb, &ret);

                state++;
            }
            break;

        case 2:
            // Looking for the media player stream. The provided name will
            // should be set as the application.name property.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                if (ret < 0) {
                    fprintf(stderr, "module-loopback failed to load\n");
                    goto error;
                }

                pa_op = pa_context_get_sink_input_info_list(
                    pa_ctx, find_stream_cb, &stream_index);

                state++;
            }
            break;

        case 3:
            // Moving the specified playback stream to the new virtual sink,
            // identified by its index, to the audiosync virtual sink.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                if (stream_index == PA_INVALID_INDEX) {
                    fprintf(stderr, "application stream couldn't be found\n");
                    ret = -1;
                    goto error;
                }

                pa_op = pa_context_move_sink_input_by_name(
                    pa_ctx, stream_index, SINK_NAME, operation_cb, &ret);

                state++;
            }
            break;

        case 4:
            // Last state, will exit.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                if (ret < 0) {
                    fprintf(stderr, "failed to move the streams\n");
                    goto error;
                }

                goto finish;
            }
            break;

        default:
            // Unexpected state
            fprintf(stderr, "Unexpected state %d\n", state);
            ret = -1;
            goto error;
        }

        // Performing the next iteration. The second argument indicates to
        // block until something is ready to be done.
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }

finish:
    // If the function got to this point, it means that it was successful.
    ret = 0;
    use_default = 0;

error:
    // Freeing all the allocated resources. No need to do so for the mainloop
    // API.
    if (pa_ctx) pa_context_disconnect(pa_ctx);
    if (pa_ctx) pa_context_unref(pa_ctx);
    if (pa_ml) pa_mainloop_free(pa_ml);

    return ret;
}

void *capture(void *arg) {
    struct ffmpeg_data *data = arg;

    // Finally starting to record the audio with ffmpeg. If the setup function
    // was called and it was successful, the audiosync monitor is used.
    // Otherwise, the default monitor will record the entire device audio.
    fprintf(stderr, "audiosync: using %s\n",
            use_default ? "default monitor" : "custom monitor");
    char *args[] = {
        "ffmpeg", "-y", "-to", MAX_SECONDS_STR, "-f", "pulse", "-i",
        use_default ? "default" : (SINK_NAME ".monitor"), "-ac",
        NUM_CHANNELS_STR, "-r", SAMPLE_RATE_STR, "-f", "f64le", "pipe:1", NULL
    };
    ffmpeg_pipe(data, args);

    pthread_exit(NULL);
}
