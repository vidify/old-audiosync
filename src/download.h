#ifndef _H_DOWNLOAD
#define _H_DOWNLOAD

#include <stdlib.h>

extern int download(char *url, double *data, size_t length, char *sample_rate,
                    char *num_channels);

#endif
