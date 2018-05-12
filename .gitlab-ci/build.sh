#!/bin/bash

set -e
set -u

export CPPFLAGS='-std=gnu99 -Wall -Wextra -Wshadow -Werror -Wno-deprecated-declarations'

./autogen.sh
./configure --enable-all
make
make install
