#pragma once

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

// Information about the audio tracks. Both must have the same formats for
// the analysis to work.
//
// The _STR macros are also required to save time when using them in a string,
// so it must be the same as the original value.
#define NUM_CHANNELS 1
#define NUM_CHANNELS_STR "1"
#define SAMPLE_RATE 48000
#define SAMPLE_RATE_STR "48000"
// The value of the last interval in audiosync.c in seconds.
#define MAX_SECONDS_STR "30"

// Conversion factor from WAV samples with the defined sample rate to
// milliseconds.
#define FRAMES_TO_MS (1000.0 / (double) SAMPLE_RATE)

// The minimum cross-correlation coefficient accepted.
#define MIN_CONFIDENCE 0.95

// Assertion that only takes place in debug mode. It helps prevent errors,
// while not affecting performance in a release.
#ifdef NDEBUG
# define DEBUG_ASSERT(x) do {} while(0)
#else
# define DEBUG_ASSERT(x) assert(x)
#endif

// Easily ignoring warnings about unused variables. Some functions in this
// module are callbacks, meaning that the parameters are required, but not all
// of them have to be used mandatorily.
#define UNUSED(x) (void)(x)

// Structure used to pass the parameters to the threads.
struct ffmpeg_data {
    const char *title;         // Only used to download the audio
    double *buf;               // Buffer with the obtained data
    size_t len;                // Current buffer's length
    const size_t total_len;    // Maximum length of the buffer
    const size_t *intervals;   // Intervals in which the data will be obtained
    const size_t n_intervals;  // Maximum number of intervals
};

// The global status variable to communicate between threads and control
// audiosync externally.
typedef enum {
    IDLE_ST,     // Audiosync is doing nothing, it's not running
    RUNNING_ST,  // Audiosync is running
    PAUSED_ST,   // Audiosync is paused (recording and downloading too)
    ABORT_ST     // Audiosync is stopping its algorithm completely
} global_status_t;
extern volatile global_status_t global_status;
// Converting a status enum value to a string.
extern char *status_to_string(global_status_t status);

// The global mutex and condition variables to synchronize between threads.
extern pthread_mutex_t mutex;
// Condition used to know when either thread has finished one of their
// intervals.
extern pthread_cond_t interval_done;
// Condition used to continue the ffmpeg execution after it has been paused
// with audiosync_pause().
extern pthread_cond_t read_continue;

// The module can be controlled externally with these basic functions. They
// expose the global status variable, which will be received from the threads
// accordingly. These functions atomically read or write the global status.
extern void audiosync_abort();
extern void audiosync_pause();
extern void audiosync_resume();
extern global_status_t audiosync_status();

// The debug mode can also be configured as a boolean, atomically.
extern volatile int global_debug;
extern int audiosync_get_debug();
extern void audiosync_set_debug(int do_debug);

// Easily and consistently printing logs to stderr.
#define DEBUG_COLOR "\x1B[36m"
#define END_COLOR "\x1B[0m"
// The ## notation will ignore __VA_ARGS__ if no extra arguments were passed
// when calling the macro. This idiom will only work on gcc and clang.
#define LOG(str, ...) \
    do { \
        if (global_debug) { \
            fprintf(stderr, DEBUG_COLOR "audiosync: " END_COLOR str "\n", \
                    ##__VA_ARGS__); \
        } \
    } while (0);

// The setup function is optional. It will initialize the PulseAudio sink to
// later record the media player output directly, rather than the entire
// desktop audio.
// Thus, the `stream_name` variable indicates the name of the music player
// being used. For example, "Spotify".
//
// It's possible that the setup fails, so it returns an integer which will
// be zero on success, and negative on error.
extern int audiosync_setup(const char *stream_name);

// Main function to start the audio synchronization algorithm. It will return
// 0 in case of success, or -1 otherwise. `yt_title` is the name of the song
// currently playing on the computer. The obtained lag will be returned to
// the variable `lag` points to.
//
// It will start two threads: one to download the audio, and another one to
// record it. These threads will signal this main function once they have
// finished an interval, so that the audio synchronization algorithm can
// be ran with the current data. This will be done until an acceptable
// result is obtained, or until all intervals are finished.
//
// This function starts the algorithm. Only one audiosync thread can be
// running at once.
extern int audiosync_run(const char *yt_title, long int *lag);
