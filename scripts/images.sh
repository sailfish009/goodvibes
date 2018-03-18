#!/bin/bash -e

CMD=$1
SVGDIR=data/icons/src

usage() {
    echo "Usage: $(basename $0) <icons/site>"
    exit 0
}

fail() {
    echo >&2 "$@"
    exit 1
}

checkcmd() {
    command -v $1 >/dev/null 2>&1 || fail "'$1' is not installed, aborting"
}

do_icons() {
    local icondir=data/icons/hicolor

    for size in 16 22 24 32 48; do
        inkscape
          --export-area-page
          --export-width $size
          --export-png $icondir/${size}x${size}/apps/goodvibes.png
          $SVGDIR/goodvibes-small.svg
    done

    for size in 256 512; do
	inkscape
          --export-area-page
          --export-width $size
	  --export-png $icondir/${size}x${size}/apps/goodvibes.png
          $SVGDIR/goodvibes-large.svg
    done
}

do_site() {
    true
}

checkcmd inkscape

[ -d $SVGDIR ] || fail "Directory '$SVGDIR' does not exist"

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
