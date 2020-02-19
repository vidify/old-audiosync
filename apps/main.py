#!/usr/bin/env python3

import sys
import audiosync


# Checking that the script was used correctly
if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} \"SONG_NAME\" [SINK_NAME]\n"
          "After running the script, start the song in the background.")
    exit(1)

# After this is printed, the music should start playing in the background too
# Running the audiosync setup in case the sink name was provided
if len(sys.argv) == 3:
    print(f"Setting up audiosync with sinkname {sys.argv[1]}")
    audiosync.setup()
# And finally the actual algorithm is ran
print("Running audiosync")
ret, success = audiosync.run(sys.argv[1])
print(f"Obtained lag (success={success}): {ret}")
