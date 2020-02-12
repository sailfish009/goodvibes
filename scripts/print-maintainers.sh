#!/bin/bash
# vim: et sts=4 sw=4

# Print the list of maintainers for distro packages

set -e
set -u


## repology backend - only gives email addresses though

REPOLOGY_API_URL=https://repology.org/api/v1
REPOLOGY_QUERY_URL=$REPOLOGY_API_URL/project/goodvibes
REPOLOGY_JSON=    # set below

repology_get_maintainers() {
    local target=$1
    local repo=
    local maint=

    repo=$(echo "$REPOLOGY_JSON" | jq ".[] | select(.repo == \"$target\")")
    if [ -z "$repo" ]; then
        echo "repo not found"
        return
    fi

    maint=$(echo "$repo" | jq '.maintainers')
    if [ "$maint" == "null" ]; then
        echo "maintainers not found"
        return
    fi

    echo "$maint" | jq -r '.[]'
#    echo "$JSON" | jq -r ".[] | select(.repo == \"$target\") | .maintainers | .[]"
}

repology_query() {
    REPOLOGY_JSON=$(wget -q -O- $REPOLOGY_QUERY_URL)
    echo " * $(repology_get_maintainers aur) - Archlinux"
    echo " * $(repology_get_maintainers debian_unstable) - Debian"
    echo " * $(repology_get_maintainers fedora_rawhide) - Fedora"
    echo " * $(repology_get_maintainers opensuse_multimedia_apps_tumbleweed) - openSUSE"
}


## http backend

AUR_URL=https://aur.archlinux.org

http_get_aur_maintainer() {
    local pkgbuild=
    local maint=

    pkgbuild=$(wget -q -O- "$AUR_URL/cgit/aur.git/plain/PKGBUILD?h=goodvibes")
    maint=$(echo "$pkgbuild" | sed -n 's/^# Maintainer: *//p')

    echo "$maint"
}

DEBIAN_URL=https://salsa.debian.org

http_get_debian_maintainer() {
    local control=
    local maint=

    control=$(wget -q -O- "$DEBIAN_URL/debian/goodvibes/raw/master/debian/control")
    maint=$(echo "$control" | sed -n 's/Maintainer: *//p')

    echo "$maint"
}

FEDORA_URL=https://src.fedoraproject.org

http_get_fedora_maintainer() {

    # Changelog headline, as found in the .spec file:
    # * Thu Feb 06 22:29:07 CET 2020 Robert-André Mauchin <zebob.m@gmail.com> - 0.5.1-1

    local spec=
    local changelog=
    local latest_entry=
    local maint=

    spec=$(wget -q -O- "$FEDORA_URL/rpms/goodvibes/raw/master/f/goodvibes.spec")
    changelog=$(echo "$spec" | sed -n '/%changelog/,$ p')
    latest_entry=$(echo "$changelog" | grep '^*' | head -1)
    maint=$(echo "$latest_entry" | cut -d' ' -f 8- | rev | sed 's/^.* - //' | rev)

    echo "$maint"
}

OPENSUSE_URL=http://download.opensuse.org

http_get_opensuse_maintainer() {

    # Changelog headline, as displayed per the rpm command:
    # * Thu Sep 26 2019 Alexei Podvalsky <avvissu@yandex.by>

    local tmpdir=
    local rpmpkg=
    local changelog=
    local latest_entry=
    local maint

    tmpdir=$(mktemp -d)
    pushd "$tmpdir" >/dev/null
    trap "popd >/dev/null && rm -fr $tmpdir" EXIT

    wget \
        --quiet \
        --recursive \
        --level=1 \
        --no-parent \
        --no-directories \
        --accept-regex 'goodvibes-.*\.src\.rpm$' \
        "$OPENSUSE_URL/repositories/multimedia:/apps/openSUSE_Tumbleweed/src/"

    rpmpkg=$(ls -1 *.rpm)
    changelog=$(rpm -q --changelog $rpmpkg 2>/dev/null)
    latest_entry=$(echo "$changelog" | grep '^*' | head -1)
    maint=$(echo "$latest_entry" | cut -d' ' -f 6-)

    trap - EXIT
    popd > /dev/null
    rm -fr "$tmpdir"

    echo "$maint"
}

http_query() {
    echo " * $(http_get_aur_maintainer) - Archlinux"
    echo " * $(http_get_debian_maintainer) - Debian"
    echo " * $(http_get_fedora_maintainer) - Fedora"
    echo " * $(http_get_opensuse_maintainer) - openSUSE"
}


## helpers

strip_email() {
    sed 's/<.*@.*> *//'
}


## main

#repology_query
http_query | strip_email
