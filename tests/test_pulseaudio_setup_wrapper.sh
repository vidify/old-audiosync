#!/usr/bin/env sh

# Shell test used to make sure that the pulseaudio setup works correctly. It's
# written in sh because that way, setting up the required streams and such
# for the test is much easier.
#
# The test_pulseaudio_setup.c file contains a simple interface for the
# setup, that takes the stream name as a parameter.
#
# The test will restart pulseaudio at the end, so it may modify your current
# pulseaudio's setup.
#
# Two tests will be performed:
#     * The first one will create the audiosync monitor and set-up everything
#     * The second one will do the same. In this case, a new sink shouldn't
#       be created. Instead, the previous sink will be re-used.

STREAM="stream1"
SINK="audiosync"
CURDIR=$(dirname "$0")

# First of all, restarting pulseaudio
killall pulseaudio
if ! pulseaudio --check &>/dev/null; then
    pulseaudio --start -D
fi

# Creating a dummy application with the known stream name
pactl load-module module-null-sink sink_name="dummy" sink_properties=device.description="dummy" 1>/dev/null
pacmd load-module module-loopback source="dummy.monitor" sink_input_properties="application.name=$STREAM" 1>/dev/null
num_sinks=$(pactl list short sinks | wc -l)

# Runinng the pulseaudio setup
echo "==== First run ===="
"$CURDIR/test_pulseaudio_setup" "$STREAM"

# Only one new sink should be created
if [ "$((num_sinks + 1))" -ne $(pactl list short sinks | wc -l) ]; then
    echo "Sink creation unsuccessful"
    exit 1
fi

# Checking that the sink was created
if ! pacmd list-sinks | grep "<$SINK>" -B 3 -q; then
    echo "Sink not found"
    exit 1
fi

# Making sure that the input was correctly moved
if ! pacmd list-sink-inputs | grep "$STREAM" -B 20 | grep "sink:" | grep "<$SINK>" -q; then
    echo "Stream not moved to the required sink"
    exit 1
fi

# A second test to make sure that no new sinks are created, meaning that
# the previous one is being re-used.
echo "==== Second run ===="
"$CURDIR/test_pulseaudio_setup" "$STREAM"

# Same test, making sure that no new sinks were created.
if [ "$((num_sinks + 1))" -ne $(pactl list short sinks | wc -l) ]; then
    echo "Sink not being reused"
    exit 1
fi

# Checking again
if ! pacmd list-sinks | grep "<$SINK>" -B 3 -q; then
    echo "Sink not found"
    exit 1
fi

# Making sure that the input was correctly moved again
if ! pacmd list-sink-inputs | grep "$STREAM" -B 20 | grep "sink:" | grep "<$SINK>" -q; then
    echo "Stream not moved to the required sink"
    exit 1
fi

# Restarting pulseaudio at the end to undo the changes made.
killall pulseaudio
pulseaudio --start -D
exit 0
