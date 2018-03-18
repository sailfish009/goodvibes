#!/bin/bash -e

CMD=$1
SVGDIR=data/icons/src
ICONDIR=data/icons/hicolor
SITEDIR=docs/goodvibes.readthedocs.io

usage() {
    echo "Usage: $(basename $0) <icons/site>"
    echo
    echo "This script is used to re-build various images out of the svg sources"
    echo
    exit 0
}

fail() {
    echo >&2 "$@"
    exit 1
}

checkcmd() {
    command -v $1 >/dev/null 2>&1 || fail "Command '$1' is not installed, aborting"
}

checkdir() {
    [ -d "$1" ] || fail "Directory '$1' does not exist, aborting"
}

do_icons() {
    checkdir $ICONDIR

    echo '--- Building small icons ---'
    for size in 16 22 24 32 48; do
        inkscape \
            --export-area-page \
            --export-width $size \
            --export-png $ICONDIR/${size}x${size}/apps/goodvibes.png \
            $SVGDIR/goodvibes-small.svg
    done

    echo '--- Building large icons ---'
    for size in 256 512; do
	inkscape \
            --export-area-page \
            --export-width $size \
	    --export-png $ICONDIR/${size}x${size}/apps/goodvibes.png \
            $SVGDIR/goodvibes-large.svg
    done

    echo '--- Copying symbolic icons ---'
    cp $SVGDIR/goodvibes-symbolic.svg $ICONDIR/symbolic/apps/goodvibes-symbolic.svg
}

do_site() {
    checkdir $SITEDIR

    echo '--- Building favicon ---'
    tmpdir=$(mktemp --directory --tmpdir=$(pwd) favicon.XXXXXX)
    trap "rm -fr $tmpdir" EXIT
    for size in 16 24 32 48 64; do
	inkscape \
            --export-area-page \
            --export-width $size \
            --export-png $tmpdir/$size.png \
	    $SVGDIR/goodvibes-favicon.svg
    done
    convert $tmpdir/*.png $SITEDIR/images/favicon.ico
    identify $SITEDIR/images/favicon.ico

    echo '--- Building goodvibes logo ---'
    inkscape \
	--export-area-page \
	--export-width 192 \
	--export-png $SITEDIR/images/goodvibes.png \
        $SVGDIR/goodvibes-large.svg
}

checkcmd convert
checkcmd identify
checkcmd inkscape
checkdir $SVGDIR

case $CMD in
    icons)
	do_icons
	;;

    site)
	do_site
	;;

    *)
	usage
	;;
esac
