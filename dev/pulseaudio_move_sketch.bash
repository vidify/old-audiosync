#!/usr/bin/env bash 
# This script is a sketch of what happens inside audiosync's pulseaudio
# implementation. It creates a new sink referencing the Spotify audio to only
# record the Spotify audio, rather than the entire desktop. That way, more
# complex pulseaudio setups are also supported.
# See https://github.com/vidify/audiosync/issues/22 for more.

STREAMS="spotify"
SINKNAME="audiosync" 

# Move spotify streams to output to new virtual device 
streams_to_move=$(pacmd list-sink-inputs | grep -B 25 "$STREAMS" | grep "index:")
echo "Moving stream ${streams_to_move:11} to $SINKNAME"
pacmd move-sink-input "${streams_to_move:11}" "$SINKNAME"
