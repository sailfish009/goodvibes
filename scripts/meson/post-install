#!/bin/sh

set -e

DATADIR="$1"

# Package managers set this so we don't need to run
if [ "$DESTDIR" ]; then exit 0; fi

cmdcheck() { command -v "$1" >/dev/null 2>&1; }

if cmdcheck glib-compile-schemas; then
  echo "Compiling GSettings schemas..."
  glib-compile-schemas "$DATADIR/glib-2.0/schemas"
fi

if cmdcheck gtk-update-icon-cache; then
  echo "Updating icon cache..."
  gtk-update-icon-cache -t -f "$DATADIR/icons/hicolor"
fi

if cmdcheck update-desktop-database; then
  echo "Updating desktop database..."
  update-desktop-database "$DATADIR/applications"
fi
