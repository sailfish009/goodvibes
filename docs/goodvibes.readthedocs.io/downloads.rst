Downloads
=========

.. include:: icons.rst
.. highlight:: none

The easiest way to install Goodvibes is to use the package manager of your
distribution, or to install via Flatpak.

If your distrib doesn't provide a package, or if you want to install a more
recent version, have a look below.



|flatpak-logo| Flatpak
----------------------

`Goodvibes on Flathub
<https://flathub.org/apps/details/io.gitlab.Goodvibes>`__



|debian-logo| Debian
--------------------

::

        codename=$(lsb_release -sc)
        sudo tee << EOF /etc/apt/sources.list.d/elboulangero.list
        deb http://pkg.elboulangero.com/debian ${codename:?} main
        EOF

        sudo apt-key adv --keyserver keys.gnupg.net --recv-key FFD52770DD5A8135
        sudo apt-get update
        sudo apt-get install goodvibes

At the moment, you will find packages for ``stretch`` and ``buster``.



|ubuntu-logo| Ubuntu
--------------------

::

        sudo add-apt-repository ppa:elboulangero/goodvibes
        sudo apt-get update
        sudo apt-get install goodvibes

More details at https://launchpad.net/~elboulangero/+archive/ubuntu/goodvibes.



|arch-logo| Arch Linux
----------------------

Goodvibes is available on AUR: https://aur.archlinux.org/packages/goodvibes.



|source-logo| Releases Tarballs
-------------------------------

If there is no package for your distribution, and if you have some packaging
skills, please get in touch, your contribution is welcome!

To see the list of releases, head to https://gitlab.com/goodvibes/goodvibes/tags.

For build instructions, refer to the `README <https://gitlab.com/goodvibes/goodvibes#compiling>`_.

Thanks!
