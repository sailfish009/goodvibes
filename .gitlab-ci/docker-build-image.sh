#!/bin/bash

set -e
set -u

fail() { echo >&2 "$@"; exit 1; }

[ $# -eq 1 ] || fail "Usage: $(basename $0) <dockerfile>"

DOCKERFILE=$1
[ -e $DOCKERFILE ] || fail "File '$DOCKERFILE' does not exist"

FROM=$(grep '^FROM' $DOCKERFILE | sed 's/^FROM *//')
[ "$FROM" ] || fail "Failed to parse dockerfile"

TAG=registry.gitlab.com/goodvibes/goodvibes/$FROM

sudo docker build \
    --tag $TAG \
    --file $DOCKERFILE \
    .

cat << EOF

--------

Now you might just want to push the image:

    sudo docker push $TAG

EOF
