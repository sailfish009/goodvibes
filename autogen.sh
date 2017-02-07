#!/bin/sh

# Run this before running ./configure

set -e

echo "Running $0..."

# Enable pre-commit hook (taken verbatim from systemd)
if [ -f .git/hooks/pre-commit.sample ] && [ ! -f .git/hooks/pre-commit ]; then 
        # This part is allowed to fail 
        cp -p .git/hooks/pre-commit.sample .git/hooks/pre-commit && \ 
        chmod +x .git/hooks/pre-commit && \ 
        echo "Activated pre-commit hook." || : 
fi 

# Autoreconf
autoreconf --force --install --verbose

echo "Done, please type './configure' to continue."

