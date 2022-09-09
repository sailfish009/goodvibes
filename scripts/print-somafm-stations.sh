#!/bin/bash
# vim: et sts=4 sw=4

# Print the list of SomaFM stations
#
# From https://somafm.com/about/faq.html:
# Q. What streams are the highest quality?
# A. The 128k AAC streams are the best sounding.
#
# Therefore we pick the 128k AAC.

set -e
set -u

URL=https://somafm.com
HTML=$(wget -O- $URL/listen/)
STATIONS=$(echo "$HTML" | pup "div#stations")
RESULTS=

# Assume less than 100 stations
for i in $(seq 1 100); do
    html=$(echo "$STATIONS" | pup "ul li:nth-child($i)")
    [ "$html" ] || break
    sta=$(echo "$html" | pup 'h3 json{}' | jq -r '.[].text')
    url=$(echo "$html" | pup 'nobr:contains("AAC PLS (SSL)") a:contains("128k") attr{href}')
    if ! grep -q ^SomaFM <<< $sta; then
        sta="SomaFM ${sta}"
    fi
    # Fixup: the specials station change its name,
    # depending on what's on. Keep a fixed name.
    if [ "$url" = "/specials130.pls" ]; then
        sta="SomaFM Specials"
    fi
    RESULTS="${RESULTS}${url} ${sta}"$'\n'
done

RESULTS=$(echo "$RESULTS" | sed "/^[[:space:]]*$/d" | LC_ALL=C sort -f -k2)
LENGTH=$(echo "$RESULTS" | wc -l)

echo "Found $LENGTH stations:"
echo "$RESULTS"
echo
read -r -p "Proceed?"

echo "-------- 8< --------"

echo "$RESULTS" | while read -r uri name; do
    uri=${URL}${uri}
    cat << EOF | sed -e 's/^/\t"/' -e 's/$/" \\/'
<Station>
  <name>${name}</name>
  <uri>${uri}</uri>
</Station>
EOF
done | sed '$ s/ \\$//'

echo "-------- >8 --------"
