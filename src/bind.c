// This file defines the Python bindings for the audiosync C extension.
//
// It's fairly straightforward, since all it does is convert the C types
// to Python and vice versa, so that audiosync's C functions can be used.
// The defined functions will behave the same, as if they were used in C.

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <audiosync/audiosync.h>


PyObject *audiosyncmodule_pause(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_resume(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_abort(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_status(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_setup(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_set_debug(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_get_debug(PyObject *self, PyObject *args);
PyObject *audiosyncmodule_run(PyObject *self, PyObject *args);


static PyMethodDef VidifyAudiosyncMethods[] = {
    {
        "run",
        audiosyncmodule_run,
        METH_VARARGS,
        "Obtain the provided YouTube song's lag in respect to the currently"
        " playing track. It can only be run once at a time."
    },
    {
        "pause",
        audiosyncmodule_pause,
        METH_NOARGS,
        "Pause the audiosync job. Thread-safe."
    },
    {
        "resume",
        audiosyncmodule_resume,
        METH_NOARGS,
        "Continue the audiosync job. This has no effect if it's not paused."
        " Thread-safe."
    },
    {
        "abort",
        audiosyncmodule_abort,
        METH_NOARGS,
        "Abort the audiosync job. Thread-safe."
    },
    {
        "status",
        audiosyncmodule_status,
        METH_NOARGS,
        "Returns the current job's status as a string. Thread-safe."
    },
    {
        "setup",
        audiosyncmodule_setup,
        METH_VARARGS,
        "Attempts to initialize a PulseAudio sink to record more easily the"
        " audio directly from the music player stream. Not thread-safe."
    },
    {
        "get_debug",
        audiosyncmodule_get_debug,
        METH_NOARGS,
        "Returns audiosync's debug level. Thread-safe."
    },
    {
        "set_debug",
        audiosyncmodule_set_debug,
        METH_VARARGS,
        "Sets audiosync's debug level. Thread-safe."
    },
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef audiosync = {
    PyModuleDef_HEAD_INIT,
    "audiosync",
    "The audio synchronization module for vidify.",
    -1,
    VidifyAudiosyncMethods
};


PyMODINIT_FUNC PyInit_audiosync(void) {
    return PyModule_Create(&audiosync);
}


PyObject *audiosyncmodule_run(PyObject *self, PyObject *args) {
    UNUSED(self);

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
    UNUSED(self); UNUSED(args);

    Py_BEGIN_ALLOW_THREADS
    audiosync_pause();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


PyObject *audiosyncmodule_resume(PyObject *self, PyObject *args) {
    UNUSED(self); UNUSED(args);

    Py_BEGIN_ALLOW_THREADS
    audiosync_resume();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}


PyObject *audiosyncmodule_abort(PyObject *self, PyObject *args) {
    UNUSED(self); UNUSED(args);

    Py_BEGIN_ALLOW_THREADS
    audiosync_abort();
    Py_END_ALLOW_THREADS

    Py_RETURN_NONE;
}

PyObject *audiosyncmodule_status(PyObject *self, PyObject *args) {
    UNUSED(self); UNUSED(args);

    global_status_t status;
    char *str;

    Py_BEGIN_ALLOW_THREADS
    status = audiosync_status();
    str = status_to_string(status);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("s", str);
}

PyObject *audiosyncmodule_setup(PyObject *self, PyObject *args) {
    UNUSED(self);

    char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    int ret;
    Py_BEGIN_ALLOW_THREADS
    ret = audiosync_setup(name);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("O", ret == 0 ? Py_True : Py_False);
}

PyObject *audiosyncmodule_get_debug(PyObject *self, PyObject *args) {
    UNUSED(self); UNUSED(args);

    return Py_BuildValue("O", audiosync_get_debug() ? Py_True : Py_False);
}

PyObject *audiosyncmodule_set_debug(PyObject *self, PyObject *args) {
    UNUSED(self);

    int do_debug;
    if (!PyArg_ParseTuple(args, "i", &do_debug)) {
        return NULL;
    }
    audiosync_set_debug(do_debug);

    Py_RETURN_NONE;
}
