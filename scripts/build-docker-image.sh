#!/bin/bash
# vim: et sts=4 sw=4

set -e
set -u

DOCKERFILE=    # $1

GITLAB_REGISTRY=registry.gitlab.com
GITLAB_NAMESPACE=goodvibes
GITLAB_PROJECT=goodvibes

DOCKER=docker
DOCKER_ARGS=()


## utils

usage() {
    local status=$1

    (( $status != 0 )) && exec >&2

    echo "Usage: $(basename $0) DOCKERFILE"
    echo
    echo "Build a docker image. For example:"
    echo
    echo "    export http_proxy=http://172.17.0.1:3142"
    echo "    $(basename $0) .gitlab-ci/Dockerfile.debian"
    echo

    exit $status
}

fail() {
    echo >&2 "$@"
    exit 1
}

user_in_docker_group() {
    id -Gn | grep -q '\bdocker\b'
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

user_in_docker_group || \
    DOCKER='sudo docker'

[ "${http_proxy:-}" ] && \
    DOCKER_ARGS+=(--build-arg "http_proxy=$http_proxy")

WORKDIR=$(dirname "$DOCKERFILE")
DOCKERFILE=$(basename "$DOCKERFILE")
pushd "$WORKDIR" >/dev/null
$DOCKER build \
    "${DOCKER_ARGS[@]}" \
    --tag "$IMAGE_TAG" \
    --file "$DOCKERFILE" \
    .
popd >/dev/null

cat << EOF

                ----------------

Now you might just want to push this image to the registry:

    $DOCKER login $GITLAB_REGISTRY
    $DOCKER push $IMAGE_TAG
    $DOCKER logout $GITLAB_REGISTRY

For an overview of the images present in the registry:

    https://gitlab.com/goodvibes/goodvibes/container_registry

EOF
