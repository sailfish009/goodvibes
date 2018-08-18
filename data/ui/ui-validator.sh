#!/bin/bash

set -e
set -u

UIFILE=$1

# Skip test if gtk-builder-tool is not installed
command -v gtk-builder-tool >/dev/null 2>&1 || exit 77

# Skip test if there's no graphics (ie. chroot, container, etc...)
[ "${DISPLAY:-}" ] || exit 77

gtk-builder-tool validate $UIFILE
