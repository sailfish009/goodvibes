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

assert_commands jq wget

API_URL=https://www.radiofrance.fr/fip/api
DATA=$(wget -O- $API_URL/live/webradios)
DATA=$(echo "$DATA" | jq 'sort_by(.name)')
N=$(echo "$DATA" | jq 'length')

TEMPLATE="\
<Station>
  <name>#name#</name>
  <uri>#uri#</uri>
</Station>"

echo "-------- 8< --------"

for i in $(seq 0 $((N - 1))); do
    data=$(echo "$DATA" | jq ".[$i]")
    name=$(echo "$data" | jq -r ".name")
    if [ -z "$name" ]; then
        echo "No name found for webradio index $i" >&2
        continue
    fi
    streams=$(echo "$data" | jq -r '.streams.live[] | "\(.format) \(.url)"')
    url=
    while read -r format url; do
        [ $format == hls ] || continue
        url=$url
        break
    done <<< $streams
    if [ -z "$url" ]; then
        echo "No HLS URL found for webradio index $i ($name)" >&2
        continue
    fi
    url=$(echo "$url" | cut -d"?" -f1)
    echo "$TEMPLATE" \
        | sed -e "s;#name#;$name;" -e "s;#uri#;$url;" \
        | sed -e 's/^/\t"/' -e 's/$/" \\/'
done | sed '$ s/ \\$//'

echo "-------- >8 --------"
