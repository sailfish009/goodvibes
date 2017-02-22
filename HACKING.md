HACKING
=======

Here's the place to get started when hacking Goodvibes. Additional documentation is available on the Github Wiki: <https://github.com/elboulangero/goodvibes/wiki>

Table of Contents
-----------------

1. [Download](#download)
2. [Compilation](#compilation)
3. [Program Invocation](#program-invocation)
4. [Additional Tools](#additional-tools)
5. [Code Overview](#code-overview)
6. [How To Code That](#how-to-code-that)
7. [Coding Style](#coding-style)
8. [Contribution](#contribution)



Download
--------

To get the latest source, clone the repository from GitHub.

	git clone https://github.com/elboulangero/goodvibes.git

You can grab extra resources with an helper script.

	./scripts/extra.sh clone

It will clone some related repositories, such as dockerfiles, debian package, wiki.



Compilation
-----------

Goodvibes uses the `autotools` as a build system. The procedure to compile is the usual one.

        ./autogen.sh
        ./configure
        make

If you want the build to be more verbose, invoke `./configure` as such.

        ./configure --enable-silent-rules

Goodvibes build is quite modular. Features that require an external library are compiled only if the required dependencies are found on your system. Otherwise they're excluded from the build.

You can change this behavior though. You can force a full-featured build, that will fail if some of the required dependencies are missing.

        ./configure --enable-all

Or you can do the opposite, and force a minimal build with none of the optional features enabled.

        ./configure --disable-all

On top of that, you can explicitly enable or disable any optional features, using the `--enable-FEATURE` or `--disable-FEATURE` options (and replacing *FEATURE* with a feature's name). Such parameters take precedence over the `--enable-all` and `--disable-all` options.

For example, to have a minimal build with only the UI enabled:

        ./configure --disable-all --enable-ui

To have a full-featured build but disable notifications:

        ./configure --enable-all --disable-notifications

Notice that disabling the UI will also disable all the UI-related features.

For more details, please have a look into the file [configure.ac](configure.ac), or run:

        ./configure -h



Program Invocation
------------------

#### From source (no installation)

First and before all, are you stuck with this error message ?

	[GLib-GIO] Settings schema 'com.elboulangero.Goodvibes.Core' is not installed

Running Goodvibes without installing it requires a bit of additional work. This is because of some libraries that Goodvibes use, which expect shared resources to be installed in standard locations. To be more accurate:

- GLib expects settings schema to be installed under `/usr/share/glib-2.0/schemas/` or similar.
- GTK+ expects icons to be installed under `/usr/share/icons` or similar.

If you don't want to install anything, then you need to define some environment variables. To save you from this burden, there's a little script file doing just that, and you just need to source it.

	source hacking.env

Have a look at this file for details.

#### From non-standard directory

If for some reason you install Goodvibes in a non-standard directory (`/opt` or `/home/user` or whatever), you will be hit by the same problems mentioned above: the shared resource won't be found at run-time. So please refer to the file `hacking.env` to see which environment variables you need to export before running Goodvibes.

#### Command-line options

To get a brief overview of the command-line options available, invoke with `-h`.

The option you will use the most is `-l` to change the log level. Here is a typical line:

	./src/goodvibes -l trace

Colors are enabled by default, but you can disable it with `-c` if it hurts your eyes. Colors are automatically disabled when logs are redirected to a file.

Logs are all sent to `stderr`, whatever the log level.

Internally, we use the GLib to ouput log messages. For more details, refer to the page [GLib Message Output and Debugging Functions][].

Some of the libraries we use provide additional command-line options. To see them all, invoke with `--help-all`. For more details, refer to:

- [Running GLib Applications][]
- [Running GStreamer Applications][]
- [Running GTK+ Applications][]

Hardcore GTK+ debugging can be done with [GtkInspector][]:

	./src/goodvibes --gtk-debug=interactive

[glib message output and debugging functions]: https://developer.gnome.org/glib/stable/glib-Message-Logging.html
[running glib applications]: https://developer.gnome.org/glib/stable/glib-running.html
[running gstreamer applications]: https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gst-running.html
[running gtk+ applications]: https://developer.gnome.org/gtk3/stable/gtk-running.html
[gtkinspector]: https://wiki.gnome.org/Projects/GTK+/Inspector



Additional Tools
----------------

[GSettings][] is part of GLib, and is the component in charge of handling the application settings. It comes with a command-line tool named, as you can guess, `gsettings`. This is useful to witness Goodvibes reading and writing its configuration, and also to get or set some values.

	gsettings monitor com.elboulangero.Goodvibes.Core

[DConf][] is the backend for GSettings. It's possible to play directly with the `dconf` command, therefore by-passing completely GSettings.

	dconf watch /com/elboulangero/Goodvibes/
	dconf reset -f /com/elboulangero/Goodvibes/

[gsettings]: https://developer.gnome.org/gio/stable/GSettings.html
[dconf]: https://wiki.gnome.org/Projects



Code Overview
-------------

Goodvibes is written in C/GObject. The code is heavily object-oriented, and if you're not familiar with GObject, you will have to accept that some magic is going on, here and there. Good luck with that ;)

The code is neatly split into different parts:

- `additions`: this is where I extend some of the libraries I use, where I add some functions that I wish would exist already.
- `framework`: the name says it all.
- `core`: the core of Goodvibes, basically enough to have the software up and running, without the ui.
- `ui`: the GTK+ user interface.
- `feat`: features that can be enable/disabled at compile-time. I guess it's quite similar to the plugins you often find in music players on GNU/Linux. Except that I didn't dare to call it plugin, for plugins are usually something discovered and loaded at run-time, not compile-time.
- `libcaphe`: a library to handle system sleep inhibition.

I suggest to have a look at [configure.ac](configure.ac) and [src/Makefile.am](src/Makefile.am) for more details.



How To Code That
----------------

#### Adding a new setting

Settings are mapped directly to object properties. You must ensure that:

- the object must be a global instance, part of the `core`, `ui` or `feat` system.
- the object must implement the `GvConfigurable` interface.

You must bind the property to a setting using `g_settings_bind()`. This **must NOT be done** at construct-time, but later on in the `configure()` function (implemented from the GvConfigurable interface).

You must understand that the startup procedure is made in two steps:

- first, every objects are created. No errors are expected at this stage.
- second, every objects are configured. Serious things start to happen, there might be errors, and it's important to be ready to report it to the user.

#### Reporting an error to the user

Here's how to report an error to the user from an object:

- the object must have been registered using the function `gv_framework_register()`.
- the object must implement the `GvErrorable` interface.

Then, it's just a matter of invoking the function `gv_errorable_emit_error()`.

A few things to notice:

- you shouldn't call this function too early (aka during construction), as the system is not ready yet.
- the user message **must be** translatable.
- It's up to you to drop an additional log (`WARNING()` should be used).
- the log message **must NOT be** translatable.



Coding Style
------------

Coding style matters to me. I keep the codebase as clean and neat as possible. I'm a bit of a maniac about that, I tell you.

#### Indentation

The code is currently indented using [Artistic Style][]. There's a script to automatically indent the whole thing:

	./scripts/code/indent.sh all

You can (and should) also indent your staged changes before commiting:

	./scripts/code/indent.sh staged

[artistic style]: http://astyle.sourceforge.net/

#### Comments

For comments, always use C style (aka `/* ... */`).

#### Codetags

For codetags, always use C++ style (aka `//`).

Here are the codetags in use here:

- `WISHED` Things I wish, but will probably never do.
- `TODO`   Things that should be done pretty soon.
- `FIXME`  For things obviously broken.

Always try to make it a one-liner if possible, or at least describe the problem in one line, then add more details on the following lines.

Stick to these conventions, and then getting lists of things is easy:

	# TODO list
	ack 'TODO'
	# List everything
	ack '// '

Here are some links that discuss codetags:

- <http://legacy.python.org/dev/peps/pep-0350/>
- <http://stackoverflow.com/q/1452934/776208>

#### GObject C Files Layout

If you find yourself writing a new file, therefore creating a new object, you might want to use a script that generates all the boilerplate.

	./scripts/code/gv-object-make.sh

Have a look at the structure of the C file created, and please notice a few things.

Functions are ordered from low-level to high-level, to avoid the need of declaring functions before defining it. So basically, you read the file from bottom to top.

Functions are grouped in sections, titled by a comment such as:

	/*
	 * Section name
	 */

Most of the time, these sections are always the same, because implementing a GObject always boils down to the same thing, more or less: GObject inherited methods, property accessors, signal handlers, public methods, private methods, helpers. Sometimes we implement an interface, and that's all there is to it. So I try to show this consistent layout and this simplicity in the C file, and to have it always the same way, instead of throwing code in a complete mess.

If you stick to these conventions, then it's easy to find your way in the code: whatever file you're looking at, they all look the same.



Contribution
------------

Contributing is better done through Github, so please follow the usual Github workflow:

- have your account setup
- fork the project
- work on your own version
- discuss the changes, and eventually submit a merge request

If you hack around for your own use-case, and think that other users could benefit from your hacks, feel free to share it and discuss it on Github.

If we decide that your hacks should be integrated upstream, then you will have to transform it in proper commits, make sure it fits into the current design, that it respects the coding style and so on.

Please note that my first goal for Goodvibes is to ensure a long-term maintenance. There are more than enough software with a short lifetime around, and I don't want to add another to the list. It means I want the code to be clean, readable, maintanable. Clean design and zero hacks are my goals. Dirty hacks are OK to solve a problem quickly, or to demonstrate a possible solution. But they are not OK for long-term maintenance, and therefore won't make it upstream.
