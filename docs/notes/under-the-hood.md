Under the hood
==============



Features
--------

#### Notifications

Notification support is provided by GApplication. Under the hood, two backends
exist out there: `org.freedesktop.Notifications` and `org.gtk.Notifications`.
Glib gives preference to the Gtk backend in case both are available.

As of 2018-11, it seems that notifications in Flatpak don't work, using the
Freedesktop runtime 18.08.

#### Multimedia Keybinding

Currently implemented with libkeybinder, which works at the X11 level to take
global keybindings. Obviously this won't do with Wayland.

Another possibility is to use D-Bus: `org.gnome.SettingsDaemon.MediaKeys`,
but this is GNOME-specific as you can see (I think there's also a Mate-specific
`org.mate.SettingsDaemon` D-Bus service).

There's a discussion at <https://github.com/pithos/pithos/issues/299>

#### Sleep inhibition

Gtk+ provides everything through GtkApplication, however when I came accross
it, I had already implemented my stuff through an internal lib "libcaphe". So,
well... Looking into details, I saw that Gtk+ implementation doesn't try to
inhibit with `org.freedesktop.PowerManagement` (I think it's been deprecated
for a while).  However at the time I was using a fairly old Openbox system,
which had only this service running to provide sleep inhibition.

So I kept my own implementation in libcaphe, and I don't use Gtk+ sleep
inhibition.
