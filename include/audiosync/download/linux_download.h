#pragma once

// Function used for the download thread. In this case, it both obtains the
// direct youtube link to download the audio from, and creates a new
// PulseAudio process to save it inside the thread's data.
//
// In case of error, it will signal the main thread to abort.
void *download(void *);

// Obtains the YouTube audio link with Youtube-dl.
//
// Returns 0 on exit, or -1 on error.
int get_audio_url(const char *title, char **url);
