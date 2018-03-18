Goodvibes was originally inspired by [RadioTray][] from Carlos Ribeiro. The original intention was to have an application that sits in the *notification area* (also called *systray*), without any main window to speak of, just a popup window. However, this notification area has been under big changes over the last years, and is quite a restricted place now. Not suitable for Goodvibes purposes anymore. But let's tell a bit of the story.

For a long time, application relied on [GtkStatusIcon][] to display an icon in the notification area. With time came the need for a successor. Ubuntu finally came up with [Application Indicators][], based on existing work by KDE. You can read more about it at the [Application Panel Indicators][] page. However, GNOME had their own view on the question, and didn't adopt appindicators. In the end, GNOME finally [deprecated][] GtkStatusIcon in GTK+ 3.14, before completely [removing it][] in GTK+ 4.0.

If you're interested in this piece of history, there's quite a long discussion on the matter at [The Libappindicator Story][].

Well, so what's the situation now ?

- GNOME (and possibly others) doesn't allow applications to access the notification area.
- Ubuntu (along with most of the other desktops) provides access to the notification area through appindicators. Applications can display an icon, and popup a menu when the icon is clicked. No distinction is made between left or right click.
- Old-school desktops still rely on GtkStatusIcon, which allows displaying anything when clicked (not limited to a menu), and supports any mouse action (left/right/middle click, mouse scrolling).

The conclusion to be drawn from that is that the notification area is now *optional*. An application shouldn't rely on it to function. It should use it only for convenience when it's available, but should do just as well without it.

Back to Goodvibes: sitting in the system tray is a thing of the past, let's acknowledge the fact. That's why Goodvibes is now a very standard application, with a main window, eating some space in your panel even if you don't like it.



[radiotray]: http://radiotray.sourceforge.net/
[gtkstatusicon]: https://developer.gnome.org/gtk3/stable/GtkStatusIcon.html
[application indicators]: https://unity.ubuntu.com/projects/appindicators/
[application panel indicators]: https://wiki.ubuntu.com/DesktopExperienceTeam/ApplicationIndicators
[deprecated]:  https://git.gnome.org/browse/gtk+/commit/?id=cab7dcde1bef1ea589a9f3c4d8512e59ade8a4a2
[removing it]: https://git.gnome.org/browse/gtk+/commit/?id=d2a8667f835ddce4ac88b9bee143556f9efa93c4
[the libappindicator story]: https://bethesignal.org/blog/2011/03/12/the-libappindicator-story/
