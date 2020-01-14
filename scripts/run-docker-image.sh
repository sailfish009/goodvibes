#!/bin/bash
# vim: et sts=4 sw=4

set -e
set -u

IMAGE=    # $1

DOCKER=docker


## utils

usage() {
    local status=$1

    (( $status != 0 )) && exec >&2

    echo "Usage: $(basename $0) IMAGE"

    exit $status
}

fail() {
    echo >&2 "$@"
    exit 1
}

user_in_docker_group() {
    id -Gn | grep -q '\bdocker\b'
}


## main

[ $# -eq 1 ] || usage 1

IMAGE=$1

user_in_docker_group || \
    DOCKER='sudo docker'

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
