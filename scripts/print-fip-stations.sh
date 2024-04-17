#!/bin/bash
# vim: et sts=4 sw=4

# Print the list of FIP stations

set -e
set -u

cmdcheck() { command -v "$1" >/dev/null 2>&1; }

assert_commands() {
    local missing=()
    local cmd=

    for cmd in "$@"; do
        cmdcheck $cmd || missing+=($cmd)
    done

    [ ${#missing[@]} -eq 0 ] && return 0

    echo "Missing command(s): ${missing[@]}" >&2
    echo "Please install it and retry." >&2
    exit 1
}

## main

assert_commands jq pup wget

URL=https://www.radiofrance.fr
HTML=$(wget -O- $URL/fip)
STATIONS=$(echo "$HTML" | pup 'a attr{href}' | grep "^/fip/radio-")

STATIONS=$(echo "$STATIONS" | LC_ALL=C sort)
LENGTH=$(echo "$STATIONS" | wc -l)

echo "Found $LENGTH stations:"
echo "$STATIONS"
echo
read -r -p "Proceed?"

echo "-------- 8< --------"

# Shouldn't be hardcoded...
cat << EOF | sed -e 's/^/\t"/' -e 's/$/" \\/'
<Station>
  <name>Fip</name>
  <uri>https://stream.radiofrance.fr/fip/fip_hifi.m3u8</uri>
</Station>
EOF

for s in $STATIONS; do
    html=$(wget -q -O- ${URL}${s})
    data=$(echo "$html" \
        | pup 'script[type="application/ld+json"]:first-of-type json{}' \
        | jq -r .[].text | jq '."@graph"[0]')
    name=$(echo "$data" | jq -r .mainEntity.name)
    # Remove ':'. However fancy utf-8 spaces makes life difficult...
    name=$(echo "$name" | sed "s/[^a-z]*:[^A-Z]*/ /")
    uri=$(echo "$data" | jq -r .audio.contentUrl | cut -d'?' -f1)
    # Not yet the final url... Assume that the latest is highest quality.
    hifi=$(wget -q -O- $uri | grep '\.m3u8$' | tail -1)
    uri=${uri%/*}/${hifi}
    cat << EOF | sed -e 's/^/\t"/' -e 's/$/" \\/'
<Station>
  <name>${name}</name>
  <uri>${uri}</uri>
</Station>
EOF
done | sed '$ s/ \\$//'

echo "-------- >8 --------"
