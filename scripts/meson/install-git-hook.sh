#!/bin/bash

set -e
set -u

cd "$MESON_SOURCE_ROOT"

[ -f .git/hooks/pre-commit.sample ] || exit 0
[ ! -f .git/hooks/pre-commit ] || exit 0

install -m0755 .git/hooks/pre-commit.sample .git/hooks/pre-commit
echo 'Git pre-commit hook installed'
