#!/bin/bash
# vim: et sts=4 sw=4

# Print the list of maintainers for distro packages

set -e
set -u

cmdcheck() { command -v "$1" >/dev/null 2>&1; }

assert_commands() {
    local missing=()
    local cmd=

    for cmd in "$@"; do
        cmdcheck $cmd || missing+=($cmd)
    done

    [ ${#missing[@]} -eq 0 ] && return 0

    echo "Missing command(s): ${missing[@]}" >&2
    echo "Please install it and retry." >&2
    exit 1
}

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
    echo " * $(repology_get_maintainers aur) - Arch Linux"
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

    # Changelog headline, as found in the changelog file:
    # * Thu Jul 22 2021 Fedora Release Engineering <releng@fedoraproject.org> - 0.6.3-2
    # * Thu Feb 06 22:29:07 CET 2020 Robert-André Mauchin <zebob.m@gmail.com> - 0.5.1-1

    local changelog_url=
    local changelog=
    local entries=
    local entry=
    local maint=

    changelog_url=$FEDORA_URL/rpms/goodvibes/raw/rawhide/f/changelog
    changelog=$(wget -q -O- "$changelog_url")
    entries=$(echo "$changelog" | grep '^*')

    while read -r entry; do
        maint=$(echo "$entry" | sed 's/^.*20[2-9][0-9]  *//' | rev | sed 's/^.* -  *//' | rev)
        case "$maint" in
            ("Fedora Release Engineering"*) continue ;;
        esac
        echo "$maint"
        return
    done <<< $entries
}

NIXOS_URL=https://raw.githubusercontent.com/NixOS/nixpkgs

http_get_nixos_maintainer() {

    local recipe=
    local maint=

    recipe=$(wget -q -O- "$NIXOS_URL/nixos-unstable/pkgs/applications/audio/goodvibes/default.nix")
    maint=$(echo "$recipe" | grep 'maintainers = ' | sed -E 's/.*\[ (.*) \].*/\1/')

    echo "$maint"
}

OPENSUSE_URL=https://download.opensuse.org

http_get_opensuse_maintainer() {

    # Changelog headline, as displayed per the rpm command:
    # * Thu Sep 26 2019 Alexei Podvalsky <avvissu@yandex.by>

    local tmpdir=
    local rpmpkg=
    local changelog=
    local latest_entry=
    local maint=

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

VOID_URL=https://raw.githubusercontent.com/void-linux/void-packages

http_get_void_maintainer() {

    local template=
    local maint=

    template=$(wget -q -O- "$VOID_URL/master/srcpkgs/goodvibes/template")
    maint=$(echo "$template" | sed -n 's/^maintainer=//p')
    maint=$(echo "$maint" | sed -e 's/^"*//' -e 's/"*$//')

    if echo "$maint" | grep -q 'orphan@voidlinux.org'; then
        # package doesn't have a maintainer
        return
    fi

    echo "$maint"
}

http_query() {
    local maint=

    maint=$(http_get_aur_maintainer)
    [ -n "$maint" ] && echo " * $maint - Arch Linux"
    maint=$(http_get_debian_maintainer)
    [ -n "$maint" ] && echo " * $maint - Debian"
    maint=$(http_get_fedora_maintainer)
    [ -n "$maint" ] && echo " * $maint - Fedora"
    maint=$(http_get_nixos_maintainer)
    [ -n "$maint" ] && echo " * $maint - NixOS"
    maint=$(http_get_opensuse_maintainer)
    [ -n "$maint" ] && echo " * $maint - openSUSE"
    maint=$(http_get_void_maintainer)
    [ -n "$maint" ] && echo " * $maint - Void Linux"
}


## helpers

strip_email() {
    sed 's/<.*@.*> *//'
}


## main

assert_commands jq rpm wget

#repology_query
http_query | strip_email
