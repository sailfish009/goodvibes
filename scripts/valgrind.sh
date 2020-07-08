#!/bin/bash

# https://stackoverflow.com/a/4256967

set -e
set -u

LOGFILE=valgrind-$(date +%s)

echo "Valgrind log file: $LOGFILE"
echo "Starting up, please be patient..."

export G_DEBUG=gc-friendly
export G_SLICE=always-malloc

valgrind \
	--leak-check=yes \
	--log-file=$LOGFILE \
	--trace-children=yes \
	--suppressions=/usr/share/gtk-3.0/valgrind/gtk.supp \
	--suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
	--suppressions=$HOME/src/gnome/libsoup/tests/libsoup.supp \
	--suppressions=$HOME/src/fdo/gstreamer/tests/check/gstreamer.supp \
	--suppressions=$HOME/src/fdo/gst-plugins-bad/tests/check/gst-plugins-bad.supp \
	--suppressions=$HOME/src/fdo/gst-plugins-base/tests/check/gst-plugins-base.supp \
	--suppressions=$HOME/src/fdo/gst-plugins-good/tests/check/gst-plugins-good.supp \
	./goodvibes-launcher.sh "$@"

# Remove pid, so that it's easy to compare two log files
cut -d' ' -f2- $LOGFILE > $LOGFILE.tmp
mv $LOGFILE.tmp $LOGFILE
