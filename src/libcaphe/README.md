Libcaphe
========

Libcaphe is a little library that inhibits sleep/suspend.

You use it in your application if you want to prevent the system from going to
sleep. A typical use-case is for audio/video players, who might want to inhibit
sleep while playing.

The code is inspired by [Caffeine-ng](https://gitlab.com/hobarrera/caffeine-ng).

Libcaphe will work if one of the following D-Bus services is present on your
system:

- `org.gnome.SessionManager`
- `org.xfce.SessionManager`
- `org.freedesktop.PowerManagement`
- `org.freedesktop.login1`

Notice that the prefered way to inhibit for GTK applications is to use
`gtk_application_inhibit()`. I use this library mainly to support old systems
that rely on `org.freedesktop.PowerManagement`, which has been deprecated a
while ago, but is still widely used.



About the name
--------------

"Cà phê" is the Vietnamese for coffee.

At the moment of this writing, Vietnam is the second largest producer in the
world after Brazil, and I thought the world ought to know :)
