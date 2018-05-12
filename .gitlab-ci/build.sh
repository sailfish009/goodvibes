#!/bin/bash

set -e
set -u

# no-deprecated-declarations is needed as long as we use GtkStatusIcon
# no-cast-function-type is needed for GFunc casts, typically:
#     g_list_foreach(list, (GFunc) g_object_unref, NULL);
export CPPFLAGS='\
 -std=gnu99 \
 -Wall \
 -Wextra \
 -Wshadow \
 -Werror \
 -Wno-deprecated-declarations \
 -Wno-cast-function-type'

./autogen.sh
./configure --enable-all
make
make install
