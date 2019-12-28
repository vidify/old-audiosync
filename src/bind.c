#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "audiosync.h"


static PyMethodDef AudiosyncMethods[] = {
	{"get_lag", audiosync_get_lag, METH_VARARGS, "Obtain the provided YouTube"
     " song's lag in respect to the currently playing track."},
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
