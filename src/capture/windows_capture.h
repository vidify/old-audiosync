#ifndef _H_WINDOWS_CAPTURE
#define _H_WINDOWS_CAPTURE

// TODO: https://docs.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording?redirectedfrom=MSDN
int capture_audio(int maxSamples, double *data);

#endif /* _H_WINDOWS_CAPTURE */
