.. title:: Goodvibes - A Lightweight Radio Player for GNU/Linux

.. toctree::
   :maxdepth: 1
   :hidden:
   :titlesonly:

   screenshots
   installation
   user-guide
   changelog
   credits

.. image:: images/goodvibes.png
   :align: center



**Goodvibes** is a lightweight internet radio player for GNU/Linux. Save your
favorite stations, play it, that's it.

There is no function to search radio stations, you'll have to enter the URL of
the audio stream by yourself. Not very user-friendly, I know, but doing better
than that is not simple.



Features
--------

The main interface is quite stripped-down, but there's a couple settings and
features waiting for those who dare to open the *Preferences* window!

 * **multimedia keys** support - ie. the ``<Play/Pause>``, ``<Previous>`` and
   ``<Next>`` keys that are present on most keyboards.
 * **notifications** - ie. information that pops up when the song change.
 * **do not suspend** - ie. prevent the system from suspending while a radio is
   playing.
 * **autoplay** - ie. start playing the last radio when you start the application.
 * **MPRIS2** support - ie. integration with modern desktops.

And for those of you who live mainly in a terminal and avoid graphical user
interfaces, you should know that you can build Goodvibes without GUI, and
control it via the command-line client provided.



Under the hood
--------------

Goodvibes is free software, released under the `GPLv3
<https://www.gnu.org/licenses/gpl-3.0.html>`_.

It relies on various free and open-source libraries. The core building blocks
are provided by `GLib <https://wiki.gnome.org/Projects/GLib>`_, the HTTP bits
are handled by `LibSoup <https://wiki.gnome.org/Projects/libsoup>`_, the audio
part is left to `GStreamer <https://gstreamer.freedesktop.org>`_ and `FFmpeg
<https://www.ffmpeg.org/>`_, and the graphical user interface is written with
`GTK <https://www.gtk.org>`_.

**Development** takes place on `GitLab <https://gitlab.com/goodvibes/goodvibes>`_,
and this is where you should head if you want to get in touch. Arnaud R is the
main author, and is the one writing these lines right now.

**Translation** happens on `Weblate <https://hosted.weblate.org/projects/goodvibes>`_.
If you want to contribute to the translation effort, don't be shy,
`get involved <https://hosted.weblate.org/engage/goodvibes>`_!

**Documentation** is hosted by `Read the Docs <https://goodvibes.readthedocs.io>`_
on this very page (don't click, you're already there).

**Artwork** is made by Hector Lahminèwskï. Feel free to visit his homepage
`Lahminewski Lab <https://lahminewski-lab.net>`_ and get in touch.



Similar Projects
----------------

 * `GRadio <https://github.com/haecker-felix/gradio>`_ is a GTK3 app for finding
   and listening to internet radio stations.
 * `Pithos <https://pithos.github.io>`_ is a native Pandora Radio client for
   Linux.
 * `RadioTray <http://radiotray.sourceforge.net>`_ (unmaintained) was an online
   radio streaming player that runs on a Linux system tray.
