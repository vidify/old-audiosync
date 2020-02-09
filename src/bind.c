#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <vidify_audiosync/audiosync.h>


PyObject *audiosyncmodule_pause(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_resume(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_abort(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_status(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_run(PyObject *self, PyObject *args);


static PyMethodDef VidifyAudiosyncMethods[] = {
    {
        "run",
        audiosyncmodule_run,
        METH_VARARGS,
        "Obtain the provided YouTube song's lag in respect to the currently"
        " playing track."
    },
    {
        "pause",
        audiosyncmodule_pause,
        METH_NOARGS,
        "Pause the audiosync job."
    },
    {
        "resume",
        audiosyncmodule_resume,
        METH_NOARGS,
        "Continue the audiosync job. This has no effect if it's not paused."
    },
    {
        "abort",
        audiosyncmodule_abort,
        METH_NOARGS,
        "Abort the audiosync job."
    },
    {
        "status",
        audiosyncmodule_status,
        METH_NOARGS,
        "Returns the current job's status as a string"
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


PyObject *audiosyncmodule_run(PyObject *self, PyObject *args) {
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


PyObject *audiosyncmodule_pause(PyObject *self, PyObject *args) {
    Py_BEGIN_ALLOW_THREADS
    audiosync_pause();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


PyObject *audiosyncmodule_resume(PyObject *self, PyObject *args) {
    Py_BEGIN_ALLOW_THREADS
    audiosync_resume();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


PyObject *audiosyncmodule_abort(PyObject *self, PyObject *args) {
    Py_BEGIN_ALLOW_THREADS
    audiosync_abort();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

PyObject *audiosyncmodule_status(PyObject *self, PyObject *args) {
    global_status_t status;
    char *str;

    Py_BEGIN_ALLOW_THREADS
    status = audiosync_status();
    str = status_to_string(status);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("s", str);
}
