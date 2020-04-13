#!/usr/bin/env python3

# Very simple test to make sure that the Python bindings work correctly.
# This will simply call the functions and do a basic test to make sure
# they 

import threading

import audiosync

print(">> Enabling/disabling debug mode")
assert(not audiosync.get_debug())
audiosync.set_debug(True)
assert(audiosync.get_debug())
audiosync.set_debug(False)
assert(not audiosync.get_debug())
audiosync.set_debug(True)

print(">> Calling setup function")
audiosync.setup("test")

# The provided arguments won't actually work inside audiosync, but it's
# enough to check if any Python errors are raised.
print(">> Starting main thread")
th1 = threading.Thread(target=audiosync.run, args=("",))
th1.start()

print(">> Pausing thread")
audiosync.pause()
status = audiosync.status()
print(">> Current status is", status)
assert(status == 'paused')

print(">> Resuming thread")
audiosync.resume()
status = audiosync.status()
print(">> Current status is", status)
assert(status == 'running')

print(">> Aborting thread")
audiosync.abort()
th1.join()
status = audiosync.status()
print(">> Current status is", status)
assert(status == 'idle')

# Testing that both threads won't overlap
print(">> Launching second thread")
th2 = threading.Thread(target=audiosync.run, args=("test",))
th2.start()

print(">> Aborting second thread")
audiosync.abort()
th2.join()
