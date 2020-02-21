#pragma once

#include "audiosync.h"


// Executes the ffmpeg command in the arguments and pipes its data into the
// provided array.
//
// It will send signals to the main thread as the intervals are being
// finished, while also checking the current global status, or updating it in
// case of errors.
//
// Returns -1 in case of error, or zero otherwise.
int ffmpeg_pipe(struct ffmpeg_data *data, char *args[]);
