#!/bin/bash
# vim: et sts=4 sw=4

set -e
set -u

DOCKERFILE=    # $1

GITLAB_REGISTRY=registry.gitlab.com
GITLAB_NAMESPACE=goodvibes
GITLAB_PROJECT=goodvibes

USAGE="Usage: $(basename $0) DOCKERFILE

Build a container image. For example:

    export http_proxy=http://localhost:3142
    ./scripts/$(basename $0) .gitlab-ci/Dockerfile.debian
"

fail() {
    echo >&2 "$@"
    exit 1
}

make_image_tag() {
    local dockerfile=$1
    local registry_image=
    local from=

    if [ "${CI_REGISTRY_IMAGE:?}" ]; then
        registry_image=$CI_REGISTRY_IMAGE
    else
        # Cf. https://docs.gitlab.com/ee/user/packages/container_registry/
        # section "Naming convention for your container images"
        registry_image=$GITLAB_REGISTRY/$GITLAB_NAMESPACE/$GITLAB_PROJECT
    fi

    from=$(sed -n 's/^FROM  *//p' "$dockerfile")
    [ "$from" ] || return 0

    echo $registry_image/$from
}


## main

[ $# -eq 1 ] || fail "$USAGE"

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

[ "${GITLAB_CI:-}" = true ] && exit 0

cat << EOF

                ----------------

Now you might want to push this image to the registry:

    podman login $GITLAB_REGISTRY
    podman push $IMAGE_TAG
    podman logout $GITLAB_REGISTRY

For an overview of the images present in the registry:

    https://gitlab.com/goodvibes/goodvibes/container_registry

EOF
