#pragma once

// Function used for the capture thread. It will start a new ffmpeg process
// to record either the custom sink created with pulseaudio_setup, or the
// entire desktop.
//
// In case of errors, it will signal the main thread to abort.
void *capture(void *);

// Creates a new dedicated sink for recording within this module. It's
// useful for complex PulseAudio setups, and avoids recording the entire
// desktop audio. Only the indicated stream will be recorded, where
// `stream_name` is the `application.name` attribute of its pulseaudio's
// stream.
//
// If this function isn't called, the capture function will use the desktop
// audio instead, so it's completely optional, although recommended.
//
// This function will return 0 if it ran successfully, or -1 in case something
// went wrong.
int pulseaudio_setup(const char *stream_name);
