Goodvibes Radio Player
======================

[![Tag](https://img.shields.io/github/tag/elboulangero/goodvibes.svg)](https://github.com/elboulangero/goodvibes/releases)
[![Build](https://img.shields.io/shippable/586380d4666b4b1000d180e8.svg)](https://app.shippable.com/github/elboulangero/goodvibes)
[![Translation](https://hosted.weblate.org/widgets/goodvibes/-/svg-badge.svg)](https://hosted.weblate.org/engage/goodvibes/?utm_source=widget)
[![License](https://img.shields.io/github/license/elboulangero/goodvibes.svg)](COPYING)

Goodvibes is a lightweight internet radio player for GNU/Linux. It offers a
simple way to have your favorite radio stations at easy reach.

<p align="center">
  <img src="https://raw.githubusercontent.com/elboulangero/goodvibes/master/data/icons/hicolor/256x256/apps/goodvibes.png" alt="Goodvibes logo"/>
</p>

Goodvibes provides everything you can expect from a modern media player:
multimedia keys binding, mpris2 support, notifications, and sleep inhibition.
It can be launched with or without GUI, and comes with a command-line client.

Under the hood, Goodvibes relies on proven and approved open-source libraries:
[GLib][] at the core, [GStreamer][] to handle the audio, and [GTK+][] for the
graphical user interface.

The project is hosted on Github at <https://github.com/elboulangero/goodvibes>.

The documentation is available at <https://github.com/elboulangero/goodvibes/wiki>.

[glib]:      https://wiki.gnome.org/Projects/GLib
[gstreamer]: https://gstreamer.freedesktop.org/
[gtk+]:      https://www.gtk.org/



Table of Contents
-----------------

* [Installation From A Package Manager](#installation-from-a-package-manager)
* [Installation From Source](#installation-from-source)
* [Dependencies](#dependencies)
* [License](#license)
* [Translations](#translations)
* [Hacking](#hacking)



Installation From A Package Manager
-----------------------------------

The easiest way to install Goodvibes is to use an existing package for your
distribution.

#### Debian

You can install from my Debian repository (only `stretch` and `buster` are
supported).

	codename=$(lsb_release -sc)
	sudo tee << EOF /etc/apt/sources.list.d/elboulangero.list
	deb http://pkg.elboulangero.com/debian ${codename:?} main
	EOF

	sudo apt-key adv --recv-key FFD52770DD5A8135

	sudo apt-get update && \
	sudo apt-get install goodvibes

#### Ubuntu

There's a PPA for Ubuntu users available on Launchpad. To install Goodvibes
from there, please visit the following page and follow the instructions:
<https://launchpad.net/~elboulangero/+archive/ubuntu/goodvibes>

#### ArchLinux

There's an ArchLinux package available:
<https://aur.archlinux.org/packages/goodvibes/>



Installation From Source
------------------------

If there's no package for you, you will need to download the source code and
compile by yourself.

At first, grab the source code using git.

	git clone https://github.com/elboulangero/goodvibes.git

Compilation is done with the [Autotools][] using the usual set of commands.

	./autogen.sh && \
	./configure && \
	make

Installation is a one-liner, and must be run as root.

	sudo make install

Of course, you need to install the required dependencies.

[autotools]: https://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html



Dependencies
------------

Compile-time dependencies are:

- For the core: `glib`, `gstreamer`, `libsoup`
- For the user interface: `gtk+`, `libkeybinder`

Additional run-time dependencies are listed below.

For more gory details, please refer to the file [configure.ac](configure.ac).

#### Install dependencies on Debian

Compile-time dependencies:

	sudo apt install \
	    autoconf autopoint build-essential \
	    libglib2.0-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libsoup2.4-dev \
	    libgtk-3-dev libkeybinder-3.0-dev

Additional run-time dependencies (probably already installed):

	sudo apt install dconf-gsettings-backend   # to load/store the settings
	sudo apt install gstreamer1.0-plugins-ugly # to listen to mp3 streams
	sudo apt install gstreamer1.0-pulseaudio   # for pulseaudio users
	sudo apt install gstreamer1.0-alsa         # for alsa users



License
-------

Copyright (C) 2015-2017 Arnaud Rebillout <elboulangero@gmail.com>.

Artwork by Hector Lahminèwskï <h.lahminewski@gmail.com>, you can visit his
homepage at <http://lahminewski-lab.net>.

Goodvibes is released under the [GNU General Public License (GPL) version
3](COPYING).



Translations
------------

Goodvibes uses [Weblate][] to manage translations. If you want to help
translating Goodvibes, please visit the project page at
<https://hosted.weblate.org/projects/goodvibes>.

[weblate]: https://weblate.org



Hacking
-------

Please refer to the [HACKING](HACKING.md) file to get started.
