Under the hood
==============



Features
--------

#### Notifications

Notification support is provided by GApplication. Under the hood, two backends
exist out there: `org.freedesktop.Notifications` and `org.gtk.Notifications`.
Glib gives preference to the Gtk backend in case both are available.

2018-11, Flatpak, Freedesktop runtime `18.08`, notifications don't seem to
work.

2020-01, Flatpak, Freedesktop runtime `19.08`, notifications work.

#### Multimedia Keybinding

Currently implemented with libkeybinder, which works at the X11 level to take
global keybindings. Obviously this won't do with Wayland.

Another possibility is to use D-Bus: `org.gnome.SettingsDaemon.MediaKeys`,
but this is GNOME-specific as you can see (I think there's also a Mate-specific
`org.mate.SettingsDaemon` D-Bus service).

There's a discussion at <https://github.com/pithos/pithos/issues/299>

But the truth is: keybinding is actually not needed if the desktop already has
some kind of MPRIS2 support, and also grabs the multimedia keys by default.
Which is the case for GNOME and most modern desktops I suspect. In this case,
when a multimedia key is pressed, it's intercepted by GNOME, which forwards
that to the current MPRIS2 player active.

So the multimedia keybinding feature in Goodvibes is mostly useful for stripped
down desktop environments with no MPRIS2 support. And in this case it's useless
to try the GNOME daemon, so we're just fine implementing X11 keybinding only.
