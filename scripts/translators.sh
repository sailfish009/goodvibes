#!/bin/bash -e

# Print the list of translators

fail() {
    echo >&2 "$@"
    exit 1
}

[ -d "po" ] || fail "'po' directory not found. Please run this script from the src root."

for file in po/*.po; do
    lang=$(basename $file | cut -d'.' -f1)
    line=$(grep '^"Last-Translator' $file)

    # xargs here is used to trim whitespaces
    # https://stackoverflow.com/a/12973694/776208
    name=$(echo $line | cut -d':' -f2- | cut -d'<' -f1 | xargs)
    email=$(echo $line | cut -d'<' -f2- | cut -d'>' -f1 | xargs)

    # Normal output for console
    #echo "$name <$email> ($lang)"

    # C style output, to be pasted directly in the code
    echo "	\"$name <$email> ($lang)\n\" \\"
done
