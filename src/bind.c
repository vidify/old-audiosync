#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <vidify_audiosync/audiosync.h>

PyObject *audiosync_get_lag(PyObject *self, PyObject *args);

static PyMethodDef VidifyAudiosyncMethods[] = {
    {
        "get_lag",
        audiosync_run,
        METH_VARARGS,
        "Obtain the provided YouTube song's lag in respect to the currently"
        " playing track."
    },
    {
        "pause",
        audiosync_pause,
        METH_NOARGS,
        "Pause the audiosync job."
    },
    {
        "continue",
        audiosync_continue,
        METH_NOARGS,
        "Continue the audiosync job. This has no effect if it's not paused."
    },
    {
        "abort",
        audiosync_abort,
        METH_NOARGS,
        "Abort the audiosync job."
    },
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

PyObject *audiosync_get_lag(PyObject *self, PyObject *args) {
    char *yt_title;
    if (!PyArg_ParseTuple(args, "s", &yt_title)) {
        return NULL;
    }

    int ret;
    long int lag;
    Py_BEGIN_ALLOW_THREADS
    ret = audiosync_run(yt_title, &lag);
    Py_END_ALLOW_THREADS

    // Returns the obtained lag and the function exited successfully.
    return Py_BuildValue("lO", lag, ret == 0 ? Py_True : Py_False);
}
