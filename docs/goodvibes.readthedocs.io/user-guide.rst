User Guide
==========

This user guide is still a work in progress, and for now it's just about
documenting advanced tips and tricks.



Keyboard Shortcuts
------------------

Here are the shortcuts that you can use when the main menu has the focus:

 * ``<Space>`` to start/stop the playback.
 * ``<Esc>`` to minimize the mindow.

Additionally, if you click on the menus, you will see that the more common
actions are associated with a shortcut.

At last, don't forget that you can enable the multimedia keys. The setting can
be found under *Preferences -> Controls -> Keyboard*, and it is disabled by
default. You should know that multimedia keys are a global shortcut, and can be
claimed by only one application at a time: first come, first served.



GStreamer Output Pipeline
-------------------------

This setting can be configured under *Preferences -> Misc -> Playback*. It's
quite a powerful thing, but not for the faint of heart. If you want to
experiment with that, you should install the `GStreamer Tools
<https://gstreamer.freedesktop.org/documentation/tutorials/basic/gstreamer-tools.html>`_,
run a quick ``man gst-launch-1.0`` to warm you up, and then start experimenting
with ``gst-launch-1.0``.

Here are a few examples of what you can achieve with that.

Use ALSA for the audio output, and use the second soundcard
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Command-line::

        gst-launch-1.0 \
          souphttpsrc location='http://direct.fipradio.fr/live/fip-midfi.mp3' ! \
          decodebin ! \
          alsasink device=hw:2

Goodvibes setting::

        alsasink device=hw:2

See `GstAlsaSink <https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-plugins/html/gst-plugins-base-plugins-alsasink.html>`_ for details.

Delay the audio output
^^^^^^^^^^^^^^^^^^^^^^

Command-line::

        gst-launch-1.0 \
          souphttpsrc location='http://direct.fipradio.fr/live/fip-midfi.mp3' ! \
          decodebin ! \
          autoaudiosink ts-offset=5000000000

Goodvibes setting::

        autoaudiosink ts-offset=5000000000

See `GstAutoAudioSink ts-offset <https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good/html/gst-plugins-good-plugins-autoaudiosink.html#GstAutoAudioSink--ts-offset`_ for details.




Command-Line And Scripting
--------------------------

Goodvibes can be used and abused with the command-line.

The command ``goodvibes-client`` gets you started, and provides basic control
over Goodvibes. The main purpose of this client is actually to ease development
and debug, and that's why it's a bit rough on the edge, and doesn't offer
everything that is provided by the user interface. But it's still good enough
for people who prefer the command-line over a graphical ui.

However, if you want to write some scripts that interact with Goodvibes, I
recommend **NOT TO** use the client, which might be modified anytime, and
doesn't promise any backward compatibility. Instead, you should make good use
of the D-Bus interfaces implemented by Goodvibes.

Goodvibes implements and exposes two interfaces on D-Bus (assuming you didn't
disable it in the preferences):

 * the native D-Bus interface is actually the one used by ``goodvibes-client``
   to communicate with Goodvibes. This interface might be modified anytime a
   new version is released, there's no plan to maintain any compatibility over
   different versions.
 * the MPRIS2 D-Bus interface is an interface implemented by many media players
   on GNU/Linux.

To get you started with D-Bus, you should know that there are different dbus
command-line tools around:

 * ``dbus-send`` is the most basic tool, provided with the dbus daemon.
 * ``gdbus`` is provided by GLib.
 * ``qdbus`` is provided by Qt.
 * ``busctl`` is provided by Systemd.

If you plan to write scripts that interact with Goodvibes, I recommend to use
the MPRIS2 interface as much as possible, mainly for two reasons:

 * It's well defined, stable over time, therefore your script will not break
   when a new version of Goodvibes is released.
 * You will be able to re-use your work with any other media player that
   support the MPRIS2 interface. And there's a lot of them.

You should refer to the `MPRIS2 Specification <https://specifications.freedesktop.org/mpris-spec/latest>`_
for more details.

A quick example with ``qdbus``::

        qdbus org.mpris.MediaPlayer2.Goodvibes \
          /org/mpris/MediaPlayer2 \
          org.mpris.MediaPlayer2.Player.Metadata



Conky Example
-------------

Here's a snippet to integrate Goodvibes in
`Conky <http://conky.sourceforge.net/documentation.html>`_::

  ${if_match "${execi 10 qdbus org.mpris.MediaPlayer2.Goodvibes /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.PlaybackStatus}" == "Playing"}\
  Goodvibes Info
  Station:${execpi 10 qdbus org.mpris.MediaPlayer2.Goodvibes /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Metadata | grep "^goodvibes:station:" | cut -d':' -f3-}
  Artist - Track:${execpi 10 qdbus org.mpris.MediaPlayer2.Goodvibes /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Metadata | grep "^xesam:title:" | cut -d':' -f3-}\
  ${else}\
  Goodvibes is not playing\
  ${endif}

There are a few things to notice here:

 * **Don't use the native interface** ``com.elboulangero.Goodvibes``, use the
   MPRIS2 interface instead, ie. ``org.mpris.MediaPlayer2.Goodvibes``. The
   reason being that when you query Goodvibes on its native interface, it will
   be automatically launched by D-Bus in case it's not running.
 * If for some reason you really need to use the native interface, then use a
   D-Bus command-line tool that allows you to disable auto-starting the
   service. As far as I know, you can only do that with ``busctl --user
   --auto-start=no``.

One last word: I'm not a Conky guru, don't quote me on that snippet above ;)

If you can do better please share on this wiki.



Custom User-Agent
-----------------

If for some reasons you need to use a different user-agent for a specific radio
station, it's possible. You just need to edit the file where the stations are
stored (usually ``~/.config/goodvibes/stations``), and add a field
``<user-agent>`` to the station you want to customize. For example::

        <Station>
          <uri>http://example.com/radio</uri>
          <name>Example Radio</name>
          <user-agent>Custom/1.0</user-agent>
        </Station>
