GST Tools
=========

This is a getting started to `gst-launch` and friends.



## Get started

In order to print a list of all plug-ins and elements available:

    gst-instpect-1.0

Then you might want details about a particular element:

    gst-inspect-1.0 autoaudiosink



## Play a stream

Let's get started with a curated selection of audio streams:

    AAC_STREAM=http://ice2.somafm.com/indiepop-128-aac
    MP3_STREAM=http://direct.fipradio.fr/live/fip-midfi.mp3
    OGG_VORBIS_STREAM=TODO
    OGG_OPUS_STREAM=TODO

    STREAM=$AAC_STREAM

The most simple way to play a stream:

    gst-launch-1.0 playbin uri="${STREAM:?}"

A bit more complicated, for the same result:

    gst-launch-1.0 souphttpsrc location="${STREAM:?} ! decodebin ! autoaudiosink

Let's use ALSA for the audio output, and select the second sound card:

    gst-launch-1.0 souphttpsrc location="${STREAM:?} ! decodebin ! alsasink device=hw:2

Delay the audio output (nanoseconds):

    gst-launch-1.0 souphttpsrc location="${STREAM:?} ! decodebin ! autoaudiosink ts-offset=5000000000
