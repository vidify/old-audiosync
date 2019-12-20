#ifndef _H_LINUX_CAPTURE
#define _H_LINUX_CAPTURE

extern int capture_audio(double *data, const int max_samples, int sample_rate,
                         int num_channels);

#endif /* _H_LINUX_CAPTURE */
