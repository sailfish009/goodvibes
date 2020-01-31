HACKING
=======

Here's the place to get started when hacking Goodvibes.



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

To get the latest source, clone the repository from GitLab.

    git clone https://gitlab.com/goodvibes/goodvibes.git

Additional repositories are available:

    # Debian packaging
    git clone https://gitlab.com/goodvibes/goodvibes-debian.git
    # Flatpak packaging
    git clone https://github.com/flathub/io.gitlab.Goodvibes.git



Compilation
-----------

Goodvibes is built using [Meson][] and [Ninja][]. The build commands are:

    meson build
    cd build
    ninja

(from now on, we assume that `PWD=<goodvibes-dir>/build`)

In order to install in a staging directory:

    DESTDIR=../staging ninja install

Goodvibes build is quite modular. Features that require an external library can
be disabled at build time. To see all the options available, enter the build
directory and run:

    meson configure

For example, to disable the hotkeys feature:

    meson configure -Dfeat-hotkeys=false

To disable compilation of the user interface (also disable every ui-related
features):

    meson configure -Dui=false

Additionally, it's possible to run a few tests:

    meson test

You might as well want to generate tag files for your favorite editor:

    ninja etags     # for emacs
    ninja ctags     # for vim

[meson]: http://mesonbuild.com
[ninja]: https://ninja-build.org/



Program Invocation
------------------

#### From the source tree (no installation)

If you want to run Goodvibes from the source tree, without installing it, use
the launcher script.

    ./goodvibes-launcher.sh

The script assumes that the build directory is named `build`.

Why do you need a script? Some libraries expect shared resources to be
installed in some standard locations. To be more accurate:

- GLib expects schemas to be installed at `/usr/share/glib-2.0/schemas/` or
  similar.
- GTK expects icons to be installed at `/usr/share/icons` or similar.

It's possible to customize this behavior with environment variables though, and
that's what the launcher script does. Have a look at it for details.

Also, don't expect any kind of desktop integration to work when you run from
the source tree. Ie, the desktop won't find any desktop file installed in the
standard location (`/usr/share/applications`), and various things might not
work: notifications, icon display here or there, etc.

#### From a non-standard directory

If for some reason you install Goodvibes in a non-standard directory (`/opt` or
`/home/user` or whatever), you will be hit by the same problems mentioned
above: the shared resources won't be found at run-time. So please refer to the
launcher script to see which environment variables you need to set before
running Goodvibes.

#### Command-line options

To get a brief overview of the command-line options available, use `--help`.

The option you will use the most is `-l` to change the log level. Here is a
typical line:

    goodvibes -l dbg

Colors are enabled by default, but you can disable it with `-c` if it hurts
your eyes. Colors are automatically disabled when logs are redirected to a
file.

Logs are all sent to `stderr`, regardless of the log level.

Internally, we use GLib to ouput log messages. For more details, refer to
[GLib Message Output and Debugging Functions][].

Some of the libraries we use provide additional command-line options. To see
them all, invoke with `--help-all`. For more details, refer to:

- [Running GLib Applications][]
- [Running GStreamer Applications][]
- [Running GTK Applications][]

Advanced GTK debugging can be done with the [GtkInspector][]:

    goodvibes --gtk-debug=interactive

[glib message output and debugging functions]: https://developer.gnome.org/glib/stable/glib-Message-Logging.html
[running glib applications]: https://developer.gnome.org/glib/stable/glib-running.html
[running gstreamer applications]: https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gst-running.html
[running gtk applications]: https://developer.gnome.org/gtk3/stable/gtk-running.html
[gtkinspector]: https://wiki.gnome.org/Projects/GTK/Inspector

#### Change the language

For testing, it can be convenient to change the language. For that, use the
`LANGUAGE` environment variable.

    LANGUAGE=fr goodvibes

Note that gettext will look for the message files in the path given to
`bindtextdomain(3)`, so make sure that this path contains the message files.
(hint: this path is logged when Goodvibes starts).



Additional Tools
----------------

[GSettings][] is part of GLib, and is the component in charge of handling the
application settings. It comes with a command-line tool named - as you can
guess - `gsettings`. This is useful to witness Goodvibes reading and writing its
configuration, and also to get or set some values.

    gsettings monitor io.gitlab.Goodvibes.Core
    gsettings list-recursively io.gitlab.Goodvibes.Core

[DConf][] is the backend for GSettings. It's possible to play directly with the
`dconf` command, therefore by-passing completely GSettings.

    dconf watch /io/gitlab/Goodvibes/
    dconf reset -f /io/gitlab/Goodvibes/

[GStreamer tools][] come in handy as soon as a radio station doesn't work. It's
often a good starting point to try to play a stream with `gst-launch-1.0`, and
see what comes out.

    gst-launch-1.0 playbin uri=https://direct.fipradio.fr/live/fip-midfi.mp3

[gsettings]: https://developer.gnome.org/gio/stable/GSettings.html
[dconf]: https://wiki.gnome.org/Projects/dconf
[gstreamer tools]: https://gstreamer.freedesktop.org/documentation/tutorials/basic/gstreamer-tools.html?gi-language=c



Code Overview
-------------

Goodvibes is written in C/GObject. The code is heavily object-oriented, and if
you're not familiar with GObject, you will have to accept that some magic is
going on, here and there. Good luck with that ;)

The code is neatly split into different parts:

- `framework`: the basic stuff we need.
- `core`: the core of Goodvibes, basically enough to have the software up and
  running, without the ui.
- `ui`: the GTK user interface.
- `feat`: features that can be enable/disabled at compile-time. I guess it's
  quite similar to the plugins you often find in music players on GNU/Linux.
  Except that I didn't dare to call it plugin, for plugins are usually
  something discovered and loaded at run-time, not at compile-time.
- `libcaphe`: a library to handle system sleep inhibition. Actually GTK can
  already do that through GtkApplication, so one day this will go away.



How To Code That
----------------

#### Adding a new setting

Settings are mapped directly to GObject properties. You must ensure that:

- The object must be a global instance, part of the `core`, `ui` or `feat`
  system.
- The object must implement the `GvConfigurable` interface.

You must bind the property to a setting using `g_settings_bind()`. This **must
NOT be done** at construct-time, but later on in the `configure()` function
(implemented from the GvConfigurable interface).

You must understand that the startup procedure is made in two steps:

1. Every objects are *created*. No errors are expected at this stage.
2. Every objects are *configured*. Serious things start to happen, there might
   be errors, and it's important to be ready to report it to the user. That's
   why the configure step is run *after* everything is created.

#### Reporting an error to the user

Here's how to report an error to the user from an object:

- The object must have been registered using the function
  `gv_framework_register_object()`.
- The object must implement the `GvErrorable` interface.

Then, it's just a matter of invoking the function `gv_errorable_emit_error()`.

A few things to notice:

- You shouldn't call this function too early (aka during construction), as the
  system is not ready yet.
- The user message **must be** translatable as it's intended for *users*.
- It's up to you to drop an additional log (`WARNING()` should be used).
- The log message **must NOT be** translatable as it's intended for
  *developers*.



Coding Style
------------

Coding style matters to me. I keep the codebase as clean and neat as possible.
I'm a bit of a maniac about that, I tell you.

#### Indentation

The code is currently indented using [Artistic Style][]. There's a script to
automatically indent the whole thing:

    ./scripts/code/indent.sh all

You can (and should) also indent your staged changes before commiting:

    ./scripts/code/indent.sh staged

[artistic style]: http://astyle.sourceforge.net/

#### Comments

For comments, always use C style (aka `/* ... */`).

#### Codetags

For codetags, always use C++ style (aka `//`).

Here are the codetags currently in use:

- `WISHED` Things I wish, but will probably never do.
- `TODO`   Things that should be done pretty soon.
- `FIXME`  Things obviously broken.

Always try to make it a one-liner if possible, or at least describe the problem
in one line, then add more details on the following lines.

Stick to these conventions, and then getting a list of things is easy:

    # TODO list
    ack 'TODO'
    # List everything
    ack '// '

Here are some links that discuss codetags:

- <http://legacy.python.org/dev/peps/pep-0350/>
- <http://stackoverflow.com/q/1452934/776208>

#### GObject C Files Layout

If you find yourself writing a new file, therefore creating a new object, fear
not! There's a script that generates all the boilerplate.

    ./scripts/code/gv-object-make.sh

Have a look at the structure of the C file created, and please notice a few
things.

Functions are ordered from low-level to high-level, to avoid the need of
declaring functions before defining it. So basically, you read the file from
bottom to top.

Functions are grouped in sections, titled by a comment such as:

    /*
     * Section name
     */

Most of the time, these sections are always the same, because implementing a
GObject always boils down to the same thing, more or less: GObject inherited
methods, property accessors, signal handlers, public methods, private methods,
helpers. Sometimes we implement an interface, and that's all there is to it. So
I try to show this consistent layout and this simplicity in the C file, and to
have it always the same way, instead of throwing code in a complete mess.

If you stick to these conventions, then it's easy to find your way in the code:
whatever file you're looking at, they all look the same.



Contribution
------------

Contributing is better done through GitLab, please follow the usual workflow:

- Have your account setup
- Fork the project
- Create a branch for your work
- Work
- Create a merge request

If you hack around for your own use-case, and think that other users could
benefit from your hacks, feel free to share it and discuss it on GitLab.

If we decide that your hacks should be integrated in Goodvibes proper, then you
will have to make sure it looks good, with meaningful commits that fit into the
existing design, proper commit messages, respect for the coding style, and so
on.

Please note that my first goal for Goodvibes is to ensure a long-term
maintenance. There are more than enough software with a short lifetime around,
and I don't want to add another to the list. It means I want the code to be
clean, readable, maintainable. Clean design and zero hacks are my goals. Dirty
hacks are OK to solve an outstanding problem quickly, or to demonstrate a
possible solution. But they are not OK for long-term maintenance, and therefore
won't make it upstream.
