#!/usr/bin/env bash

# This script is a sketch of what happens inside audiosync's pulseaudio
# implementation. It moves the spotify's output into the audiosync sink.
# See https://github.com/vidify/audiosync/issues/22 for more.

# SETUP PULSE
STREAMS="spotify"
SINKNAME="audiosync"

# Setup the virtual sink
if ! pacmd list-modules | grep null-sink -A 1 | grep "$SINKNAME" -q ; then
    echo -n "Loading virtual sink module as "
    pactl load-module module-null-sink sink_name="$SINKNAME" sink_properties=device.description="$SINKNAME"
else
    echo "Virtual sink module already loaded"
fi

# And setup the loopback from the monitor back to default sink
if ! pacmd list-modules | grep loopback -A 1 | grep "$SINKNAME" -q ; then
    echo -n "Loading loopback module as "
    # latency_msec is needed so that the virtual sink's audio is synchronized
    # with the desktop audio.
    pactl load-module module-loopback source="$SINKNAME.monitor" latency_msec=1
else
    echo "Loopback module already loaded"
fi

# Move spotify streams to output to new virtual device 
streams_to_move=$(pacmd list-sink-inputs | grep -B 25 "$STREAMS" | grep "index:")
echo "Moving stream ${streams_to_move:11} to $SINKNAME"
pacmd move-sink-input "${streams_to_move:11}" "$SINKNAME"
