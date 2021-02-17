Metadata
========

Metadata, or tags, everything I learnt so far.



## Plain HTTP streams, and the ICY protocol

### A bit of history

"Plain HTTP" streaming means that the client downloads an infinite file,
be it MP3, AAC or anything else. This is the "traditional" way of streaming
radio stations, as it appeared in the late 1990s.

The tricky thing is that the MP3 format only defines how to store audio data,
and it doesn't have support for metadata (or tags, ie. title, artist, album and
so on).  It's not much of a problem for a MP3 file, where metadata are either
prepended at the beggining of the file, either appended at the end. However, in
an infinite MP3 stream, how to tell the client what is the title of the track
that is being played?

The folks from Nullsoft, who were developing SHOUTcast at the time, came up
with a fairly simple solution: to embed the metadata in the audio stream at
regular interval. On the receiving end, the client just needs to make sure to
remove those chunks from the audio stream.

This is what we'll call the **ICY protocol**, as the communication between the
client and the server is based on HTTP headers starting with the prefix `icy-`
(which stands for "I Can Yell", if ever you wondered).

SHOUTcast is a proprietary solution. Icecast was released quickly afterward, as
an open-source alternative. And there are surely many more streaming solutions
around. However they all stick to the ICY protocol to send the metadata.

References:
- <https://en.wikipedia.org/wiki/SHOUTcast>
- <https://en.wikipedia.org/wiki/Icecast>
- <http://www.gigamonkeys.com/book/practical-a-shoutcast-server.html>

### How it works

Here's how it works, in a nutshell.

In the original request to the server, the client sends `icy-metadata=1`, to
say that it supports reading metadata from the stream. The server responds with
`icy-metaint=xxxx` (metadata interval), to tell to the client where to find the
metadata in the stream (it's a number of bytes). Then goes the MP3 stream, with
the metadata embedded within.

Regarding the support in GStreamer, there are two files of interest:
- `gst-plugins-good/ext/soup/gstsouphttpsrc.c` is where the HTTP headers are
  handled.
- `gst-plugins-good/gst/icydemux/gsticydemux.c` is where the metadata is
  extracted from the audio stream.

Upon reception of `icy-metaint=xxxx`, GStreamer creates the capability
`application/x-icy` with the property `metadata-interval` set accordingly.

What we can expect to see in the HTTP dialog, according to `gstsouphttpsrc.c`:
- `icy-name`  → `GST_TAG_ORGANIZATION`
- `icy-url`   → `GST_TAG_LOCATION`
- `icy-genre` → `GST_TAG_GENRE`

Then, embedded in the stream, according to `gsticydemux.c`:
- `StreamTitle` → `GST_TAG_TITLE`
- `StreamUrl`   → `GST_TAG_HOMEPAGE`

The first set of tags can be seen among the GstTagList at the beginning of
the stream, and then they quickly disappear. If an application wants to make
use of it, it needs to cache it, and it should not overwrite it with `null`
later on, when they don't appear in the GstTagList.

For more details:

    gst-inspect-1.0 icydemux
    gst-inspect-1.0 souphttpsrc

References:
- <https://gstreamer.freedesktop.org/documentation/soup/souphttpsrc.html>
- <https://gitlab.freedesktop.org/gstreamer/gst-plugins-good/blob/master/ext/soup/gstsouphttpsrc.c>
- <https://gitlab.freedesktop.org/gstreamer/gst-plugins-good/blob/master/gst/icydemux/gsticydemux.c>
- <https://www.radiotoolbox.com/community/forums/viewtopic.php?t=74>
- <https://cast.readme.io/docs/icy>

### Case studies

Radio Paradise (legacy stream):
- Stream: <https://stream.radioparadise.com/mp3-128>
- 3 ICY HTTP headers:
  - `icy-name`  : "Radio Paradise (128k mp3)"
  - `icy-url`   : "https://radioparadise.com"
  - `icy-genre` : "Eclectic"
- 2 ICY embedded metadata:
  - `StreamTitle`: "Ben Howard - In Dreams"
  - `StreamUrl`  : picture url, current song

SomaFM, Indie Pop Rocks:
- Stream: <http://ice2.somafm.com/indiepop-128-aac>
- 3 ICY HTTP headers:
  - `icy-name`  : "Indie Pop Rocks! [SomaFM]"
  - `icy-url`   : "http://somafm.com"
  - `icy-genre` : "College Indie"
- 2 ICY embedded metadata:
  - `StreamTitle`: "Starflyer 59 - Wicked Trick"
  - `StreamUrl`  : picture url, logo of the radio station

Pedro's Broadcasting Basement:
- Stream: <http://pbb.laurentgarnier.com:8000/pbb128>
- 3 ICY HTTP headers:
  - `icy-name`  : "PBB: Pedro's Broadcasting Basement"
  - `icy-url`   : "http://www.pedrobroadcast.com"
  - `icy-genre` : "Various, Eclectic"
- 1 ICY embedded metadata:
  - `StreamTitle`: "(R) THE ABYSSINIANS -SATTA MASSA GANA -MABRAK - blood and fire"

FIP (legacy stream):
- Stream: <https://direct.fipradio.fr/live/fip-midfi.mp3>
- 1 ICY HTTP header: `icy-name`: fip-midfi.mp3
- No ICY embedded metadata.

Here comes some GStreamer logs, for the sake of it:

    # SomaFM - AAC stream, with ICY
    STREAM=http://ice2.somafm.com/indiepop-128-aac
    /GstPlayBin/GstURIDecodeBin: caps = application/x-icy
    /GstPlayBin/GstURIDecodeBin: caps = audio/mpeg
    /GstPlayBin/GstURIDecodeBin: caps = audio/x-raw

    # PBB - MP3 stream, with ICY
    STREAM=http://pbb.laurentgarnier.com:8000/pbb128
    /GstPlayBin/GstURIDecodeBin: caps = application/x-icy
    /GstPlayBin/GstURIDecodeBin: caps = audio/mpeg
    /GstPlayBin/GstURIDecodeBin: caps = audio/x-raw

    # FIP (legacy) - MP3 stream, no ICY
    STREAM=https://direct.fipradio.fr/live/fip-midfi.mp3
    /GstPlayBin/GstURIDecodeBin: caps = audio/mpeg
    /GstPlayBin/GstURIDecodeBin: caps = audio/x-raw

And now a few remarks to draw from that.

We could use `icy-name` and `icy-url`, and display the name of the station as
a clickable link. However, for Goodvibes user, it's of little value, because
users already know what station they're listening to, and where to find the
homepage on the web. We can also see that the field `icy-name` can be junk,
in such case displaying it will just be annoying.

When it comes to the embedded tags, not every station sets it, far from that.
When they do, we don't know if `StreamTitle` is of the form "artist - title",
or "title - artist", or anything else really. All we can do is display it "as
is".

Interestingly, `StreamUrl` is used for the URL of a picture in some cases.
SomaFM uses it for the logo of the radio station, while Radio Paradise goes as
fas as using it for the picture of the current song. It's kind of nice, but
it's of little interest for Goodvibes, for various reasons. We could try very
hard, read this field, and if it's an URL, download it, and if it turns out to
be a picture, display it (except that right now, in the UI, pictures have
nowhere to be displayed). But heck, that's a lot of effort, and I don't even
know how widely this field is used for this purpose...

At the end of the day, the only thing supported by Goodvibes at the moment
is the embedded metadata `StreamTitle`, and I don't think that will change.

### What about OGG streams?

The reality is that OGG streams are not common. Seems like MP3 and AAC rule the
game, and ICY is the most common way to stream the metadata.

This beind said, from <https://www.radiomast.io/solutions/opus-streaming>,
there are two ways to have metadata in an OGG stream. 1) is the traditional
icy protocol, and 2) is apparently about having tags straight in the stream.

Let's check this stream:

    STREAM=http://play.global.audio/nrj_low.ogg

Indeed, we see no caps `application/icy`, and still we get tags that are
updated on a regular basis. Even though there's only a `title` tag, nothing
more.



## MPEG-DASH streams

So far I've never seen any metadata with a DASH stream.

It seems that DASH has support for metadata, and it seems that there's some
work in progress to add support in GStreamer:
- #762: add support for DASH events
  - <https://gitlab.freedesktop.org/gstreamer/gst-plugins-bad/-/issues/762>



## HLS streams

So far I've never seen any metadata with HLS streams.
