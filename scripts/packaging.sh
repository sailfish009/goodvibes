#!/bin/bash
#
# References:
# <https://wiki.debian.org/PbuilderTricks>
# <https://wiki.debian.org/cowbuilder>
# <https://help.launchpad.net/Packaging/PPA/BuildingASourcePackage>

VERSION=$1
DEBIAN_DISTS='stretch buster'
stretch=deb9
buster=deb10
UBUNTU_DISTS='xenial artful'
xenial=ubuntu16.04
artful=ubuntu17.10
ARCHIVE_BASEURL=https://github.com/elboulangero/goodvibes/archive
PACKAGE_URL=https://github.com/elboulangero/goodvibes-debian.git

fail() {
    echo >&2 $@
    exit 1
}

title() {
    echo -e '\e[7m'">>> $@"'\e[0m'
}

usage() {
    cat << EOF
Usage:   $(basename $0) <version>

Exemple: $(basename $0) 0.3.4

This script takes care of packaging Goodvibes for Debian and Ubuntu,
assuming that everything is ready: aka a new version of Goodvibes was
properly released, and the Debian packaging files have been updated,
and the whole thing's been pushed to GitHub.

Actually, the script does not deal with the current source tree (and
therefore has no real reason to live here). Everything is downloaded
from GitHub.
EOF
    exit 0
}

[ "$VERSION" ] || usage

set -e
set -u

WRKDIR=$(pwd)/packaging
mkdir $WRKDIR
cd $WRKDIR

# Download everything

title "Downloading source and packaging files ..."

URL=$ARCHIVE_BASEURL/v$VERSION.tar.gz
OUTPUT=goodvibes_$VERSION.orig.tar.gz
wget $URL -O $OUTPUT
tar -xf $OUTPUT

cd goodvibes-$VERSION

git clone $PACKAGE_URL debian
rm -fr debian/.git

DEBVERSION=$(dpkg-parsechangelog --show-field version)
[ "$VERSION" == "$(cut -d'-' -f1 <<< $DEBVERSION)" ] || \
    fail "Version '$VERSION' does not match Debian version '$DEBVERSION'"

# Build the Debian binary packages

PBUILDER_CACHE=/var/cache/pbuilder

for dist in $DEBIAN_DISTS; do
    chrootdir=$PBUILDER_CACHE/base-$dist.cow
    [ -d $chrootdir ] || continue

    title "Building binary package for $dist ..."

    sed --in-place \
	"s/$VERSION-.*) .*;/$VERSION-0~${!dist}) $dist;/" \
	debian/changelog

    sudo cowbuilder update \
	 --distribution $dist \
	 --basepath $chrootdir

    pdebuild --pbuilder cowbuilder \
	     --auto-debsign \
	     --buildresult .. \
	     -- \
	     --basepath $chrootdir
done

# Build the Ubuntu source packages

for dist in $UBUNTU_DISTS; do

    title "Building source package for $dist ..."

    sed --in-place \
	"s/$VERSION-.*) .*;/$VERSION-0~${!dist}) $dist;/" \
	debian/changelog

    debuild -S -sa
done

# Done

title "Done, time to upload !"
dput -H
