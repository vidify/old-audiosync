#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "audiosync.h"


static PyMethodDef AudiosyncMethods[] = {
	{"get_lag", audiosync_get_lag, METH_VARARGS, "Obtain the currently playing"
    "song's lag in respect to the url provided from YouTube."},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef audiosyncmodule = {
	PyModuleDef_HEAD_INIT,
	"audiosync",
	"The audio synchronization module for vidify.",
	-1,
	AudiosyncMethods
};

PyMODINIT_FUNC PyInit_audiosync(void) {
	return PyModule_Create(&audiosyncmodule);
}
