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

#URL=https://www.radiofrance.fr
#HTML=$(wget -O- $URL/fip)
#STATIONS=$(echo "$HTML" | pup 'a attr{href}' | grep "^/fip/radio-")

STATIONS="live
metal
nouveautes
world
electro
hiphop
pop
rock
groove
jazz
sacre_francais
reggae"

STATIONS=$(echo "$STATIONS" | LC_ALL=C sort)
LENGTH=$(echo "$STATIONS" | wc -l)

echo "Found $LENGTH stations:"
echo "$STATIONS"
echo
read -r -p "Proceed?"

TEMPLATE="\
<Station>
  <name>#name#</name>
  <uri>#uri#</uri>
</Station>"

echo "-------- 8< --------"

for s in $STATIONS; do
    if [ $s = live ]; then
        url="$API_URL/live?"
    else
        url="$API_URL/live/webradios/fip_$s"
    fi
    data=$(wget -q -O- "$url")
    name=$(echo "$data" | jq -r '.now.thirdLine.title')
    sources=$(echo "$data" | jq -c '.now.media.sources[]')
    url=
    for data in $sources; do
        format=$(echo "$data" | jq -r .format)
        [ $format == hls ] || continue
        url=$(echo "$data" | jq -r .url)
        break
    done
    if [ -z "$url" ]; then
        echo "No HLS URL found for stations $s!" >&2
        continue
    fi
    url=$(echo "$url" | cut -d"?" -f1)
    echo "$TEMPLATE" \
        | sed -e "s;#name#;Fip $name;" -e "s;#uri#;$url;" \
        | sed -e 's/^/\t"/' -e 's/$/" \\/'
done | sed '$ s/ \\$//'

echo "-------- >8 --------"
