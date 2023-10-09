#!/bin/bash
# vim: et sts=4 sw=4

# Print the list of Nova stations

set -e
set -u

URL=https://www.nova.fr
FILTER_OUT="bordeaux|lyon"

# Get main page
HTML=$(wget -O- $URL/radios/)

# Get auth token
AUTH_TOKEN=$(echo "$HTML" | grep "var staytunedConf = " \
    | grep -o "{.*}" | jq -r .authToken)
if [ "$AUTH_TOKEN" ]; then
    echo "Got auth token: $AUTH_TOKEN"
else
    echo "Failed to get auth token"
    exit 1
fi

# Inspect links and get the webpage for each radio
#STATIONS=$(echo "$HTML" | pup 'a attr{href}' \
#	| grep "www\.nova\.fr/radios/.\+" | LC_ALL=C sort -u)
#STATIONS=$(echo "$STATIONS" | grep -Ev "/nova-($FILTER_OUT)/")
#LENGTH=$(echo "$STATIONS" | wc -l)

# Get radio details from the header-meta-links, as json
STATIONS_JSON=$(echo "$HTML" | pup 'div[class="header-media-links"]' \
    | pup 'button json{}')
# Keep only the keys we want
STATIONS_JSON=$(echo "$STATIONS_JSON" \
    | jq 'map({"data-radioid", "data-radio-slug", "data-title"})')
# Rename those keys to make it jq-friendly
STATIONS_JSON=$(echo "$STATIONS_JSON" \
    | sed -E -e 's/"data-/"/' -e 's/radioid/id/' -e 's/radio-slug/slug/')
# Go from JSON to plain text
STATIONS=$(echo "$STATIONS_JSON" | jq -r '.[] | "\(.id) \(.slug) \(.title)"')
# Hack: rename 'Nouvo Nova' into 'Nova Nouvo' so that it sorts well
STATIONS=$(echo "$STATIONS" | sed 's/Nouvo Nova/Nova Nouvo/')
# Filter out some stations, remove duplicates, then sort per name
STATIONS=$(echo "$STATIONS" | grep -Ev " nova-($FILTER_OUT) " \
    | LC_ALL=C sort -u | sort -k3)

LENGTH=$(echo "$STATIONS" | wc -l)

echo "Found $LENGTH stations:"
echo "$STATIONS" | cut -d' ' -f3-
echo
read -r -p "Proceed?"


echo "-------- 8< --------"

while read -r id slug name; do
    #echo $id $slug $title
    json=$(wget -q -O- --header "X-Api-Key: $AUTH_TOKEN" \
        https://api.staytuned.io/v1/contents/$id)
    json=$(echo "$json" | jq '.elementList[0]')
    uri=$(echo "$json" | jq -r '.audioSrc')
    #url=$(echo "$json" | jq -r '.hdAudioSrc')
    cat << EOF | sed -e 's/^/\t"/' -e 's/$/" \\/'
<Station>
  <name>${name}</name>
  <uri>${uri}</uri>
</Station>
EOF
done <<< $STATIONS | sed '$ s/ \\$//'

echo "-------- >8 --------"
