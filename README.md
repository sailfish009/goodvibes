Goodvibes
=========

![Goodvibes Logo](raw/master/data/icons/hicolor/256x256/apps/goodvibes.png)

**Goodvibes** is an internet radio player for GNU/Linux. It aims to be light,
simple, straightforward.

- **Documentation** is available on [Read the Docs](https://goodvibes.readthedocs.io)
- **Development** is hosted on [GitLab](https://gitlab.com/goodvibes/goodvibes)
- **Translation** takes place on [Weblate](https://hosted.weblate.org/projects/goodvibes)
- **Artwork** is made by [Hector Lahminèwskï](http://lahminewski-lab.net)

Goodvibes is released under the [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html),
and relies on major open-source building blocks such as [GLib][], [LibSoup][],
[GStreamer][] and [GTK+][].

For install instructions, please refer to the
[documentation](https://goodvibes.readthedocs.io/en/latest/downloads.html).

[glib]:      https://wiki.gnome.org/Projects/GLib
[libsoup]:   https://wiki.gnome.org/Projects/libsoup
[gstreamer]: https://gstreamer.freedesktop.org/
[gtk+]:      https://www.gtk.org/



Compiling
---------

Goodvibes is written in C and builds with the Autotools.

At first, make sure you have all the dependencies installed.

*Note*: The following commands assume a Debian-like distribution. If you're
using another distribution, please adjust the package names and replace `apt`
by whatever yo use.

```bash
#
# Compile-time dependencies
#

# Build toolchain
sudo apt install autoconf autopoint build-essential git
# Core dependencies
sudo apt install libglib2.0-dev libsoup2.4-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
# GUI dependencies
sudo apt install libgtk-3-dev libkeybinder-3.0-dev

#
# Run-time dependencies
#

# To load and store settings
sudo apt install dconf-gsettings-backend
# To listen to mp3 streams
sudo apt install gstreamer1.0-plugins-ugly
# GStreamer audio backend
dpkg -s pulseaudio >/dev/null 2>&1 && \
    sudo apt install gstreamer1.0-pulseaudio || \
    sudo apt install gstreamer1.0-alsa
```

Now you're ready to get the source and compile.

```bash
# Grab the source
git clone https://gitlab.com/goodvibes/goodvibes.git
cd goodvibes

# Build
./autogen.sh
./configure
make

# Run from the source tree
./goodvibes-launcher.sh

# Install
sudo make install
```
