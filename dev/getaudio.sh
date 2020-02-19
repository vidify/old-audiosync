#!/usr/bin/env bash

FFMPEG_FLAGS="-ac 1 -r 48000"

function record() {
    echo "Recording audio into $1..."
    # Getting the default sink with pacmd list-sinks because the -i default
    # option for ffmpeg sucks.
    defaultSink=$(pacmd list-sinks | grep '*' | awk '/index/ {print $3}')
    ffmpeg -f pulse -i "$defaultSink" $FFMPEG_FLAGS "$1"
}


function download() {
    echo "Downloading $2 into $1"
    url=$(youtube-dl --get-url $1 -f bestaudio)
    echo "$url"
    ffmpeg -i "$url" $FFMPEG_FLAGS "$2"
}

if [ "$1" == "record" ]; then
    record $2
elif [ "$1" == "download" ]; then
    download $2 $3
else
    echo "$0 MODE filename.wav <parameters>"
    echo ""
    echo "MODES (only one at a time):"
    echo "    record                   record audio from the default pulseaudio"
    echo "                             output device"
    echo "    download <youtube-url>   download a youtube video"
    echo ""
    echo "examples:"
    echo "    $0 record recorded.wav"
    echo "    $0 download https://www.youtube.com/watch?v=dQw4w9WgXcQ youtube.wav"
fi
