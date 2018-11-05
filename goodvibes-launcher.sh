#!/bin/bash

# The launcher is for people who want to run Goodvibes from the source tree,
# without installing it. GLib and GTK+ need to access some shared resources,
# so we need to tell them to look for it in-tree.

fail() { echo >&2 "$@"; exit 1; }

BUILDDIR=${BUILDDIR:-build}
GOODVIBES=$BUILDDIR/src/goodvibes

[ -d $BUILDDIR ]  || fail "Build directory '$BUILDDIR' does not exit"
[ -x $GOODVIBES ] || fail "Executable '$GOODVIBES' not found"

# XDG_DATA_DIRS is needed for:
# - GTK+ to load local icons from '$BUILDDIR/data/icons'
[ "$XDG_DATA_DIRS" ] \
    && XDG_DATA_DIRS="$BUILDDIR/data:$XDG_DATA_DIRS" \
    || XDG_DATA_DIRS="$BUILDDIR/data:/usr/local/share:/usr/share"
export XDG_DATA_DIRS

# GSCHEMA_SETTINGS_DIR is needed for:
# - GLib to load local settings schemas from '$BUILDDIR/data'
# If you don't define it, you'll be hit by something like that:
#   [GLib-GIO] Settings schema '...Goodvibes.Core' is not installed
export GSETTINGS_SCHEMA_DIR="$BUILDDIR/data/"

$GOODVIBES $@
