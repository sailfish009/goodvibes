Downloads
=========

.. include:: icons.rst

The easiest way to install Goodvibes is to use the package provided by your
Linux distribution, or to install via Flatpak. Alternatively, you can also
build the application from source.



|flatpak-logo| Flatpak
----------------------

One good thing with Flatpak is that you get the latest version with minimum
effort. The package is distributed by `Flathub
<https://flathub.org/apps/details/io.gitlab.Goodvibes>`_.



|debian-logo| Debian
--------------------

Goodvibes is available in the `official Debian repositories
<https://packages.debian.org/sid/sound/goodvibes>`_ since Debian 10 "buster"
(released in July 2019).


::

        sudo apt install goodvibes

Alternatively, you can also use the packages that are available from my
`unofficial Debian repository <http://pkg.arnaudr.io>`_ for "buster" and
"sid". That will give you the latest version of Goodvibes.

::

        codename=$(lsb_release -sc)

        sudo tee << EOF /etc/apt/sources.list.d/goodvibes.list
        deb http://pkg.arnaudr.io/debian ${codename:?} goodvibes
        EOF

        sudo apt-key adv --keyserver keys.gnupg.net --recv-key FFD52770DD5A8135
        sudo apt update
        sudo apt install goodvibes



|ubuntu-logo| Ubuntu
--------------------

Goodvibes is available in the `official Ubuntu repositories
<https://packages.ubuntu.com/search?keywords=goodvibes>`_ since Ubuntu 19.04
"Disco Dingo" (released in April 2019).

::

        sudo apt install goodvibes

Alternatively, you can also use my Ubuntu `Personal Package Archive
<https://launchpad.net/~elboulangero/+archive/ubuntu/goodvibes>`_ to get the
latest version (Ubuntu 19.04 and later).

::

        sudo add-apt-repository ppa:elboulangero/goodvibes
        sudo apt update
        sudo apt install goodvibes



|fedora-logo| Fedora
--------------------

Goodvibes is available in the `Fedora updates repository
<https://bodhi.fedoraproject.org/updates/?packages=goodvibes>`_ since Fedora 30
(released in May 2019).

::

        sudo dnf install goodvibes



|opensuse-logo| openSUSE
------------------------

Goodvibes is available in the `openSUSE multimedia:apps repository
<https://software.opensuse.org/package/goodvibes>`_ since openSUSE "Leap 15.1"
(released in May 2019), and in the rolling release "Tumbleweed".

Install instructions are available in the `expert download page
<https://software.opensuse.org/download/package?package=goodvibes&project=multimedia%3Aapps>`_.

For Tumbleweed:

::

        sudo zypper addrepo https://download.opensuse.org/repositories/multimedia:apps/openSUSE_Tumbleweed/multimedia:apps.repo
        sudo zypper refresh
        sudo zypper install goodvibes



|arch-logo| Arch Linux
----------------------

Goodvibes is available in the `Archlinux User Repository
<https://aur.archlinux.org/packages/goodvibes>`_.

Alternatively, you can also use Étienne Deparis' `unofficial Archlinux
repository <https://deparis.io/archlinux/>`_ to get a binary package.



|source-logo| Releases Tarballs
-------------------------------

If there is no package for your distribution, and if you have some packaging
skills, please get in touch, your contribution is welcome!

To see the list of releases, head to https://gitlab.com/goodvibes/goodvibes/tags.

For build instructions, refer to https://gitlab.com/goodvibes/goodvibes.
