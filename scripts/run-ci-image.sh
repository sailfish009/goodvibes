#!/bin/bash
# vim: et sts=4 sw=4

set -e
set -u

fail() { echo >&2 "$@"; exit 1; }

[ $# -eq 1 ] || fail "Usage: $(basename $0) IMAGE"

IMAGE=$1

podman run \
    --rm \
    --tty \
    --interactive \
    --hostname goodvibes-builder \
    --user root:root \
    --volume $(pwd):/home/builder/goodvibes \
    --workdir /home/builder/goodvibes \
    $IMAGE \
    bash
