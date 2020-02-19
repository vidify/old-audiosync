#!/usr/bin/env python3

# Very simple test to make sure that the Python bindings work correctly.
# This will simply call the functions and do a basic test to make sure
# they 

import threading

import audiosync


print(">> Calling setup function")
audiosync.setup("test")

# The provided arguments won't actually work inside audiosync, but it's
# enough to check if any Python errors are raised.
print(">> Starting main thread")
th1 = threading.Thread(target=audiosync.run, args=("",))
th1.start()

print(">> Pausing thread")
audiosync.pause()

print(">> Resuming thread")
audiosync.resume()

print(">> Current status is", audiosync.status())

print(">> Aborting thread")
audiosync.abort()

# Testing that both threads won't overlap
print(">> Launching second thread")
th2 = threading.Thread(target=audiosync.run, args=("test",))
th2.start()

print(">> Aborting second thread")
audiosync.abort()

th1.join()
th2.join()
