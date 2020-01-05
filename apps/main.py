import sys
import vidify_audiosync as audiosync


# Checking that the script was used correctly
if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} \"SONG_NAME\"\n"
          "After running the script, start the song in the background.")
    exit(1)

# After this is printed, the music should start playing in the background too
print("Running audiosync.get_lag")
ret, success = audiosync.get_lag(sys.argv[1])
print(f"Obtained lag (success={success}): {ret}")
