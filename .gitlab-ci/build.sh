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

# As long as we use GtkStatusIcon, we need no-deprecated-declarations
CPPFLAGS="$CPPFLAGS -Wno-deprecated-declarations"

# For some casts to GFunc, we get a warning. Typically:
#
#   error: cast between incompatible function types from
#          ‘void (*)(void *)’ to ‘void (*)(void *, void *)’
#     g_list_foreach(core_objects, (GFunc) g_object_unref, NULL);
#                                  ^
# gcc prefers no-cast-function-type
# clang prefers no-bad-function-cast
# It's unclear if an agreement will be reached.
if [ "${CC:-}" == "clang" ]; then
    CPPFLAGS="$CPPFLAGS -Wno-bad-function-cast"
else
    CPPFLAGS="$CPPFLAGS -Wno-cast-function-type"
fi

export CPPFLAGS

meson build $@
ninja -C build
ninja -C build test
