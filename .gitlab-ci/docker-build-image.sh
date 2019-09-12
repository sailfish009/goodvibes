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

# Forward http proxy if found
HTTP_PROXY_BUILD_ARG=""
[ "${http_proxy:-}" ] && \
    HTTP_PROXY_BUILD_ARG="--build-arg http_proxy=$http_proxy"

# Check if we need sudo to run docker
DOCKER="docker"
id -Gn | grep -q "\bdocker\b" || \
    DOCKER="sudo docker"

# Build
$DOCKER build \
    $HTTP_PROXY_BUILD_ARG \
    --tag $TAG \
    --file $DOCKERFILE \
    .

cat << EOF

--------

Now you might just want to push the image:

    sudo docker push $TAG

EOF
