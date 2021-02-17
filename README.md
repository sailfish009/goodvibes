Goodvibes
=========

![Goodvibes Logo](https://gitlab.com/goodvibes/goodvibes/raw/master/data/icons/hicolor/256x256/apps/io.gitlab.Goodvibes.png)

**Goodvibes** is a lightweight internet radio player for GNU/Linux. Save your
favorite stations, play it, that's it.

There is no function to search radio stations, you'll have to enter the URL of
the audio stream by yourself. Not very user-friendly, I know, but doing better
than that is not simple.

Places of interest:
- **Development** is hosted here, on [GitLab](https://gitlab.com/goodvibes/goodvibes)
- **Documentation** is available on [Read the Docs](https://goodvibes.readthedocs.io)
- **Translation** takes place on [Weblate](https://hosted.weblate.org/projects/goodvibes)



## Installing

You have a few options to install Goodvibes:

1. Install the package provided by your Linux distribution (if any).
2. Install the Flatpak app available from [Flathub][].
3. Install a package from an unofficial repository.
4. Build from source (see below).

For more details, please refer to the [Installation page in the documentation][installation].

[flathub]: https://flathub.org/apps/details/io.gitlab.Goodvibes
[installation]: https://goodvibes.readthedocs.io/en/stable/installation.html



## Building from source

*Note*: The following commands are for a Debian-like distribution. If you're
using another distribution, please adjust the package names and replace `apt`
by your package manager.

*Note*: These instructions are for advanced users. Beginners should use it with
caution.

Goodvibes is written in C and builds with Meson.

At first, make sure that you have all the dependencies required.

```console
# ~ Build dependencies ~

# Build toolchain
sudo apt install build-essential git meson
# Core dependencies
sudo apt install libglib2.0-dev libsoup2.4-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
# GUI dependencies
sudo apt install libamtk-5-dev libgtk-3-dev libkeybinder-3.0-dev
# Test utils (optional)
sudo apt install appstream-util libxml2-utils

# ~ Runtime dependencies ~

# To load and store settings
sudo apt install dconf-gsettings-backend
# To play mp3 streams
sudo apt install gstreamer1.0-plugins-ugly
# To play aac streams
sudo apt install gstreamer1.0-libav
# GStreamer audio backend
dpkg -s pulseaudio >/dev/null 2>&1 && \
    sudo apt install gstreamer1.0-pulseaudio || \
    sudo apt install gstreamer1.0-alsa

# ~ Additional tooling for developers ~

# To inspect radio streams
sudo apt install gstreamer1.0-tools
```

Now you're ready to get the source code and build it:

```console
git clone https://gitlab.com/goodvibes/goodvibes.git
cd goodvibes
meson build
ninja -C build
```

Then you can run the application in-tree, without installing anything:

```console
./goodvibes-launcher.sh
```

You can also install it:

```console
sudo ninja -C build install
```



## Credits

The artwork was made by [Hector Lahminèwskï](https://lahminewski-lab.net/).

Goodvibes wouldn't exist without free and open-source software such as
[GLib][], [LibSoup][], [FFmpeg][], [GStreamer][], [GTK][] and many more.

[glib]:      https://wiki.gnome.org/Projects/GLib
[libsoup]:   https://wiki.gnome.org/Projects/libsoup
[ffmpeg]:    https://www.ffmpeg.org/
[gstreamer]: https://gstreamer.freedesktop.org/
[gtk]:       https://www.gtk.org/



## License

Goodvibes is free software, released under the [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html).
