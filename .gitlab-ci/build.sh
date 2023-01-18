#!/bin/bash

set -e
set -u
set -x

CPPFLAGS="\
 -std=gnu99 \
 -Wall \
 -Wextra \
 -Wshadow \
 -Werror"

export CPPFLAGS

meson setup build "$@"
ninja -C build
ninja -C build test
