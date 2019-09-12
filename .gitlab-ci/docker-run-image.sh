#!/bin/bash
# This script is for developers, not CI.

set -e
set -u

fail() { echo >&2 "$@"; exit 1; }

[ $# -eq 1 ] || fail "Usage: $(basename $0) <image>"

IMAGE=$1

# Check if we need sudo to run docker
DOCKER="docker"
id -Gn | grep -q "\bdocker\b" || \
    DOCKER="sudo docker"

# Run
$DOCKER run \
    --rm \
    --tty \
    --interactive \
    --hostname gvbuilder \
    --user root:root \
    --volume $(pwd):/home/builder/goodvibes \
    --workdir /home/builder/goodvibes \
    $IMAGE \
    bash
