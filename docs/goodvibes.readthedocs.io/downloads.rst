Downloads
=========

Debian
------

::

        codename=$(lsb_release -sc)
        sudo tee << EOF /etc/apt/sources.list.d/elboulangero.list
        deb http://pkg.elboulangero.com/debian ${codename:?} main
        EOF

        sudo apt-key adv --recv-key FFD52770DD5A8135
        sudo apt-get update
        sudo apt-get install goodvibes

At the moment, you will find packages for ``stretch`` and ``buster``.



Ubuntu
------

::

        sudo add-apt-repository ppa:elboulangero/goodvibes
        sudo apt-get update
        sudo apt-get install goodvibes

More details at <https://launchpad.net/~elboulangero/+archive/ubuntu/goodvibes>.



Arch Linux
----------

More details at <https://aur.archlinux.org/packages/goodvibes>.



Sources
-------

If there is no package for your distribution, and if you have some packaging
skills, please get in touch, your contribution is welcome!

Alternatively, you might want to download the source and build Goodvibes by
yourself. It's about building a C application with the Autotools.

At first, ensure that you have all the required dependencies. Here's how it
goes on Debian, you might need to adapt that a bit for your distribution::

        sudo apt-get install \
          autoconf autopoint build-essential \
          libglib2.0-dev libsoup2.4-dev \
          libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
          libgtk-3-dev libkeybinder-3.0-dev

Grab the source code from git::

        git clone https://github.com/elboulangero/goodvibes.git
        cd goodvibes

Then build::

        ./autogen.sh && \
        ./configure && \
        make

Install::

        sudo make install
