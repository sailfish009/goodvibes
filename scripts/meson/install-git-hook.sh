#!/bin/bash

set -e
set -u

cd "$MESON_SOURCE_ROOT"

[ -f .git/hooks/pre-commit ] && exit 0

install -m0755 scripts/git-hooks/pre-commit .git/hooks/pre-commit
echo 'Git pre-commit hook installed'
