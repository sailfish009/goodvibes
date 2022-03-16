#!/bin/bash

set -e
set -u

cd "$MESON_SOURCE_ROOT"

[ -d .git ] || exit 0 # not a git repo
[ -f .git/hooks/pre-commit.sample ] || exit 0 # doesn't hurt to check
[ ! -f .git/hooks/pre-commit ] || exit 0 # already installed

install -m 0755 .git/hooks/pre-commit.sample .git/hooks/pre-commit
echo 'Git pre-commit hook installed'
