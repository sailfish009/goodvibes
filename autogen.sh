#!/bin/sh
# Run this to generate all the initial makefiles, etc.

set -e

echo "Running $0..."

# Check installed commands
if ! command -v autoreconf >/dev/null 2>&1; then
        echo >&2 "*** 'autoreconf' not found, please install it ***"
        exit 1
fi

if ! command -v pkg-config >/dev/null 2>&1; then
        echo >&2 "*** 'pkg-config' not found, please install it ***"
        exit 1
fi

# Enable pre-commit hook
if [ -f .git/hooks/pre-commit.sample ] && [ ! -f .git/hooks/pre-commit ]; then
        # This part is allowed to fail
        cp -p .git/hooks/pre-commit.sample .git/hooks/pre-commit && \
        chmod +x .git/hooks/pre-commit && \
        echo "Activated pre-commit hook." || :
fi

# Autoreconf
autoreconf --force --install --verbose

# Done
echo "Done, please type './configure' to continue."

