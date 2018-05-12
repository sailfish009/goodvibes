#!/bin/bash

set -e
set -u

# no-deprecated-declarations is needed as long as we use GtkStatusIcon
# no-bad-function-cast is needed for GFunc casts, typically:
#     g_list_foreach(list, (GFunc) g_object_unref, NULL);
export CPPFLAGS="\
 -std=gnu99 \
 -Wall \
 -Wextra \
 -Wshadow \
 -Werror \
 -Wno-deprecated-declarations \
 -Wno-bad-function-cast"

./autogen.sh
./configure --enable-all
make
