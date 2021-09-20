#!/bin/bash
# vim: et sts=4 sw=4

set -e
set -u

DOCKERFILE=    # $1

GITLAB_REGISTRY=registry.gitlab.com
GITLAB_NAMESPACE=goodvibes
GITLAB_PROJECT=goodvibes


## utils

usage() {
    local status=$1

    (( $status != 0 )) && exec >&2

    echo "Usage: $(basename $0) DOCKERFILE"
    echo
    echo "Build a container image. For example:"
    echo
    echo "    export http_proxy=http://localhost:3142"
    echo "    $(basename $0) .gitlab-ci/Dockerfile.debian"
    echo

    exit $status
}

fail() {
    echo >&2 "$@"
    exit 1
}

make_image_tag() {
    local dockerfile=$1
    local from=

    from=$(sed -n 's/^FROM  *//p' "$dockerfile")
    [ "$from" ] || return

    # https://docs.gitlab.com/ee/user/packages/container_registry/#image-naming-convention
    echo "$GITLAB_REGISTRY/$GITLAB_NAMESPACE/$GITLAB_PROJECT/$from"
}


## main

[ $# -eq 1 ] || usage 1

DOCKERFILE=$1
[ -e "$DOCKERFILE" ] || fail "Dockerfile '$DOCKERFILE' does not exist"

IMAGE_TAG=$(make_image_tag "$DOCKERFILE")
[ "$IMAGE_TAG" ] || fail "Failed to make image tag from dockerfile '$DOCKERFILE'"

WORKDIR=$(dirname "$DOCKERFILE")
DOCKERFILE=$(basename "$DOCKERFILE")
pushd "$WORKDIR" >/dev/null
podman build \
    --tag "$IMAGE_TAG" \
    --file "$DOCKERFILE" \
    .
popd >/dev/null

cat << EOF

                ----------------

Now you might want to push this image to the registry:

    podman login $GITLAB_REGISTRY
    podman push $IMAGE_TAG
    podman logout $GITLAB_REGISTRY

For an overview of the images present in the registry:

    https://gitlab.com/goodvibes/goodvibes/container_registry

EOF
