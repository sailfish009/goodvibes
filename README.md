Goodvibes
=========

[![Tag](https://img.shields.io/github/tag/elboulangero/goodvibes.svg)](https://github.com/elboulangero/goodvibes/releases)
[![Build](https://img.shields.io/shippable/586380d4666b4b1000d180e8.svg)](https://app.shippable.com/github/elboulangero/goodvibes)
[![Translation](https://hosted.weblate.org/widgets/goodvibes/-/svg-badge.svg)](https://hosted.weblate.org/engage/goodvibes/?utm_source=widget)
[![License](https://img.shields.io/github/license/elboulangero/goodvibes.svg)](COPYING)

<p align="center">
  <img src="https://raw.githubusercontent.com/elboulangero/goodvibes/master/data/icons/hicolor/256x256/apps/goodvibes.png" alt="Goodvibes logo"/>
</p>

**Goodvibes** is an internet radio player for GNU/Linux. It aims to be light,
simple, straightforward.

- **Documentation** is available on [Read the Docs](https://goodvibes.readthedocs.io)
- **Translation** takes place on [Weblate](https://hosted.weblate.org/projects/goodvibes)
- **Development** is hosted on [GitHub](https://github.com/elboulangero/goodvibes)
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
git clone https://github.com/elboulangero/goodvibes.git
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
