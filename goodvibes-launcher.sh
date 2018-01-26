#!/bin/bash

# The launcher is for people who want to run Goodvibes from the source tree,
# without installing it. GLib and GTK+ need to access some shared resources,
# so we need to instruct them where to look for it.

GOODVIBES=src/goodvibes

fail() {
    echo >&2 $@
    exit 1
}

[ -x $GOODVIBES ] || fail "'$GOODVIBES' not found, did you build it?"

# XDG_DATA_DIRS is needed for:
# - GTK+ to load local icons from './data/icons'
if [ "$XDG_DATA_DIRS" ]; then
	if grep -F -q -v './data/:' <<< "$XDG_DATA_DIRS"; then
		XDG_DATA_DIRS="./data/:$XDG_DATA_DIRS"
	fi
else
	XDG_DATA_DIRS='./data/:/usr/local/share/:/usr/share/'
fi
export XDG_DATA_DIRS

# GSCHEMA_SETTINGS_DIR is needed for:
# - GLib to load local settings schemas from './data'
# If you don't define it, you'll be hit by something like that:
#   [GLib-GIO] Settings schema 'com.elboulangero.Goodvibes.Core' is not installed
export GSETTINGS_SCHEMA_DIR='./data/'

$GOODVIBES $@
