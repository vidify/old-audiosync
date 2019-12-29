#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "audiosync.h"


static PyMethodDef VidifyAudiosyncMethods[] = {
	{"get_lag", audiosync_get_lag, METH_VARARGS, "Obtain the provided YouTube"
     " song's lag in respect to the currently playing track."},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef vidify_audiosync = {
	PyModuleDef_HEAD_INIT,
	"vidify_audiosync",
	"The audio synchronization module for vidify.",
	-1,
	VidifyAudiosyncMethods
};

PyMODINIT_FUNC PyInit_vidify_audiosync(void) {
	return PyModule_Create(&vidify_audiosync);
}
