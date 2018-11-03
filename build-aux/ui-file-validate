#!/bin/sh

set -e

UIFILE="$1"

SKIP=77 # the Meson test harness recognises exit code 77 as skip

cmdcheck() { command -v "$1" >/dev/null 2>&1; }

# Skip test if there's no graphics
[ "$DISPLAY" ] || exit $SKIP

# Skip test if gtk-builder-tool is not installed
cmdcheck gtk-builder-tool || exit $SKIP

gtk-builder-tool validate "$UIFILE"
