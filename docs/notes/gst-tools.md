GST Tools
=========

This is a getting started to `gst-launch` and friends.



## Get started

In order to print a list of all plug-ins and elements available:

    gst-inspect-1.0

Then you might want details about a particular element:

    gst-inspect-1.0 autoaudiosink



## Play a stream

Let's get started with a curated selection of audio streams:

    AAC_STREAM=http://ice2.somafm.com/indiepop-128-aac
    MP3_STREAM=http://direct.fipradio.fr/live/fip-midfi.mp3
    OGG_VORBIS_STREAM=TODO
    OGG_OPUS_STREAM=TODO

    HLS_AAC_STREAM=https://stream.radiofrance.fr/fip/fip_hifi.m3u8
    DASH_STREAM=https://a.files.bbci.co.uk/media/live/manifesto/audio/simulcast/dash/nonuk/dash_low/aks/bbc_radio_one.mpd

    STREAM=$AAC_STREAM

The most simple way to play a stream:

    gst-launch-1.0 playbin uri="${STREAM:?}"

A bit more complicated, for the same result:

    gst-launch-1.0 souphttpsrc location="${STREAM:?}" ! decodebin ! autoaudiosink

Let's use ALSA for the audio output, and select the second sound card:

    gst-launch-1.0 souphttpsrc location="${STREAM:?}" ! decodebin ! alsasink device=hw:2

Delay the audio output (nanoseconds):

    gst-launch-1.0 souphttpsrc location="${STREAM:?}" ! decodebin ! autoaudiosink ts-offset=5000000000

Ignore invalid SSL certificates (for HTTPS streams):

    gst-launch-1.0 souphttpsrc ssl-strict=false location="${STREAM:?}" ! fakesink



## Debug a stream

`gst-launch-1.0` comes with a bunch of handy options, like:
- `-t` to display metadata
- `-m` to display messages
- `-v` to display a lot of details, and `-X` to exclude some properties

For full details about a stream, without continuous flooding due to tags:

    gst-launch-1.0 playbin uri="${STREAM:?}" -v -X tags

To display only the caps for this stream:

    stdbuf -oL -eL gst-launch-1.0 playbin uri="${STREAM:?}" -v | sed -n 's/.*caps = //p' | uniq
