Installation
============

.. include:: icons.rst
.. highlight:: none

The easiest way to install Goodvibes is to use the package provided by your
Linux distribution, if any. It's 100% hassle free, however you might not get
the latest version.

To get latest version, the easiest way is to install the Flatpak app. Another
option is to use an unofficial repository for your Linux distro, if any.

At last, it's always possible to grab the source code and build it.



|flatpak-logo| Flathub
----------------------

* `Flatpak app on Flathub <https://flathub.org/apps/details/io.gitlab.Goodvibes>`_

::

        flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
        flatpak install flathub io.gitlab.Goodvibes



|debian-logo| Debian
--------------------

* `Debian repository <https://packages.debian.org/sid/sound/goodvibes>`_
  (since Debian 10 "Buster")

::

        sudo apt install goodvibes

* `Unofficial Debian repository <http://pkg.arnaudr.io>`_
  (Debian 10 "Buster" and later)

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

* `Ubuntu repository <https://packages.ubuntu.com/search?keywords=goodvibes>`_
  (since Ubuntu 19.04 "Disco Dingo")

::

        sudo apt install goodvibes

* `Unofficial PPA <https://launchpad.net/~elboulangero/+archive/ubuntu/goodvibes>`_
  (Ubuntu 19.04 "Disco Dingo" and later)

::

        sudo add-apt-repository ppa:elboulangero/goodvibes
        sudo apt update
        sudo apt install goodvibes



|fedora-logo| Fedora
--------------------

* `Fedora updates repository <https://bodhi.fedoraproject.org/updates/?packages=goodvibes>`_
  (since Fedora 30)

::

        sudo dnf install goodvibes



|opensuse-logo| openSUSE
------------------------

* `openSUSE multimedia:apps repository <https://software.opensuse.org/package/goodvibes>`_
  (rolling release "Tumbleweed", stable release 15.x "Leap")

::

        # For Tumbleweed (for other releases, adjust the url)
        sudo zypper addrepo https://download.opensuse.org/repositories/multimedia:apps/openSUSE_Tumbleweed/multimedia:apps.repo
        sudo zypper refresh
        sudo zypper install goodvibes

You can find more install instructions on the `Expert Download page
<https://software.opensuse.org/download/package?package=goodvibes&project=multimedia%3Aapps>`_.



|arch-logo| Arch Linux
----------------------

* `Archlinux User Repository <https://aur.archlinux.org/packages/goodvibes>`_

::

         git clone https://aur.archlinux.org/goodvibes.git
         cd goodvibes
         makepkg -si

* `Étienne Deparis' Unofficial Archlinux User Repository <https://deparis.io/archlinux/>`_
  (for a prebuilt binary package)

::

        # Setup the repository first, then:
        pacman -S goodvibes



|source-logo| Releases Tarballs
-------------------------------

If there is no package for your distribution, and if you have some packaging
skills, please get in touch, your contribution is welcome!

For build instructions, refer to https://gitlab.com/goodvibes/goodvibes.
