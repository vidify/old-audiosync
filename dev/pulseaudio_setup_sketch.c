// Sketch file in C for audiosync. This file will set-up pulseaudio to
// record output from the media player passed as a parameter: a new virtual
// sink will be created, and the media player's stream will be moved to
// the new sink to later be able to record it easily.
//
// A more complete and updated version of this program can be found in
// src/capture/linux_capture.c
//
// It can be compiled with:
// $ gcc pulseaudio_setup_sketch.c -o pulse_setup -lpulse
//
// And then, ran with:
// $ ./pulse_setup "SINKNAME"
//
// SINKNAME can be for example Spotify. Please note that case sensitivity
// is important. You can check the name it has by looking up the
// application.name property of the sink with:
// $ pacmd list-sink-inputs | grep "$SINKNAME" -B 25 | grep "application.name"

#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>

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

// The main loop is split into the different states that will be followed.
typedef enum {
    // Check if this function has been called before (skips to 3 if true)
    LOOP_CHECK_SINK = 0,
    // Create a null sink for the audiosync extension
    LOOP_CREATE_SINK = 1,
    // Loopback into the new sink
    LOOP_LOOPBACK = 2,
    // Search the media player's stream
    LOOP_GET_STREAM = 3,
    // Move the found stream into the new sink
    LOOP_MOVE_STREAM = 4,
    // Last state before ending the loop
    LOOP_FINISHED = 5
} loop_state_t;

// The stream name is a global so that it can be accessed inside callback
// functions.
char *STREAM_NAME;


// This callback gets called when our context changes state. We really only
// care about when it's ready or if it has failed.
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

// Callback used to check if the custom monitor already exists.
static void sink_exists_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    int *exists = userdata;

    // If eol is positive it means there aren't more available streams.
    if (eol > 0) {
        return;
    }

    // Checking if the sink's name is the same. First of all, checking that
    // the provided name isn't empty.
    if (i->name == NULL) {
        return;
    }
    if (strcmp(i->name, SINK_NAME) == 0) {
        *exists = 1;
    }
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
    if (strstr(name, STREAM_NAME) != NULL) {
        *index = i->index;
    }
}

// Pulseaudio asynchronously handles all the requests, so this mainloop
// structure is needed to handle them one by one.
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s \"SINKNAME\"\n", argv[0]);
        return 1;
    }

    // Saving the stream name as a global for later.
    STREAM_NAME = strdup(argv[1]);

    // Define the pulseaudio loop and connection variables
    pa_mainloop *pa_ml;
    pa_mainloop_api *pa_mlapi;
    pa_operation *pa_op;
    pa_context *pa_ctx;

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

    // We'll need these state variables to keep track of our requests
    loop_state_t state = LOOP_CHECK_SINK;
    int sink_exists = 0;
    int ret;
    uint32_t stream_index = PA_INVALID_INDEX;
    while (1) {
        // We can't do anything until PA is ready, so just iterate the mainloop
        // and continue.
        if (server_status == PA_REQUEST_NOT_READY) {
            pa_mainloop_iterate(pa_ml, 1, NULL);
            continue;
        }

        // Error connecting to the server.
        if (server_status == PA_REQUEST_ERROR) {
            fprintf(stderr, "audiosync: error when connecting to the server.\n");
            pa_context_disconnect(pa_ctx);
            pa_context_unref(pa_ctx);
            pa_mainloop_free(pa_ml);
            return -1;
        }

        // At this point, we're connected to the server and ready to make
        // requests.
        switch (state) {
        case LOOP_CHECK_SINK:
            // Checking if there already exists a sink with the same name,
            // meaning that this function has been called more than once and
            // that this can skip directly to step number 3.
            pa_op = pa_context_get_sink_info_list(
                        pa_ctx, sink_exists_cb, &sink_exists);
            state++;
            break;

        case LOOP_CREATE_SINK:
            // This sends an operation to the server. The first one will be
            // loading a new sink for audiosync.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                // If the previous operation concluded that the custom sink
                // already exists, then it's reused, and it skips to the
                // last step related to creating the sink.
                if (sink_exists) {
                    fprintf(stderr, "audiosync: found custom monitor, reusing it\n");
                    pa_op = pa_context_get_sink_input_info_list(
                        pa_ctx, find_stream_cb, &stream_index);
                    state = LOOP_MOVE_STREAM;
                    break;
                }

                fprintf(stderr, "audiosync: no custom monitor found, creating a new one\n");
                pa_op = pa_context_load_module(
                    pa_ctx, "module-null-sink", "sink_name=" SINK_NAME
                    " sink_properties=device.description=" SINK_NAME
                    " format=float32le",
                    load_module_cb, &ret);

                state++;
            }
            break;

        case LOOP_LOOPBACK:
            // Now we wait for our operation to complete. When it's
            // complete, we move along to the next state, which is loading
            // the loopback module for the new virtual sink created.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                // Checking the returned value by the previous state's module
                // load.
                if (ret < 0) {
                    fprintf(stderr, "audiosync: module-null-sink failed to load\n");
                    return 1;
                }

                // latency_msec is needed so that the virtual sink's audio is
                // synchronized with the desktop audio (disables the delay).
                fprintf(stderr, "audiosync: new sink created, loading loopback\n");
                pa_op = pa_context_load_module(
                    pa_ctx, "module-loopback", "source=" SINK_NAME ".monitor"
                    " latency_msec=1", load_module_cb, &ret);

                state++;
            }
            break;

        case LOOP_GET_STREAM:
            // Looking for the media player stream. The provided name will
            // should be set as the application.name property.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                if (ret < 0) {
                    fprintf(stderr, "audiosync: module-loopback failed to load\n");
                    return 1;
                }

                fprintf(stderr, "audiosync: looking for the provided stream\n");
                pa_op = pa_context_get_sink_input_info_list(
                    pa_ctx, find_stream_cb, &stream_index);

                state++;
            }
            break;

        case LOOP_MOVE_STREAM:
            // Moving the specified playback stream to the new virtual sink,
            // identified by its index, to the audiosync virtual sink.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                if (stream_index == PA_INVALID_INDEX) {
                    fprintf(stderr, "audiosync: application stream couldn't be found\n");
                    return 1;
                }

                fprintf(stderr, "audiosync: moving the found stream\n");
                pa_op = pa_context_move_sink_input_by_name(
                    pa_ctx, stream_index, SINK_NAME, operation_cb, &ret);

                state++;
            }
            break;

        case LOOP_FINISHED:
            // Last state, will exit.
            if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
                if (ret < 0) {
                    fprintf(stderr, "audiosync: failed to move the streams\n");
                    return 1;
                }

                fprintf(stderr, "audiosync: setup complete\n");
                return 0;
            }
            break;

        default:
            // Unexpected state
            fprintf(stderr, "Unexpected state %d\n", state);
            return 1;
        }

        // Performing the next iteration. The second argument indicates to
        // block until something is ready to be done.
        pa_mainloop_iterate(pa_ml, 1, NULL);
    }
}
