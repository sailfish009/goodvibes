#!/bin/bash
# vim: et sts=4 sw=4

# Update the years for the copyright statements

fail() {
    echo >&2 "$@"
    exit 1
}

git diff-files --quiet || \
    fail "Working tree has unstaged changes"

git diff-index --quiet --cached HEAD -- || \
    fail "Working tree has staged changes"

TRACKED_FILES=$(git ls-tree -r HEAD --name-only)
INTERESTING_FILES=()

# AFAIK, all of this will fail miserably
# if ever a filename contains a space...
for f in $TRACKED_FILES; do
    case "$f" in
        (COPYING) continue ;;
        (*.po)    continue ;;
        (*.ico)   continue ;;
        (*.png)   continue ;;
        (*.svg)   continue ;;
        (*/font-mfizz/*) continue ;;
    esac
    INTERESTING_FILES+=("$f")
done

#echo "${INTERESTING_FILES[@]}" | tr ' ' '\n'

YEAR=$(date +%Y)
sed -i -E \
    -e "s/(Copyright \(C\) 20[0-9][0-9])-(20[0-9][0-9])/\1-$YEAR/g" \
    -e "s/(Copyright \(C\) 20[0-9][0-9]) /\1-$YEAR /g" \
    ${INTERESTING_FILES[@]}
