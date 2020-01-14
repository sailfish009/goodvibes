#!/bin/bash
# vim: et sts=4 sw=4

# Rebuild png images

set -e
set -u

APPID=io.gitlab.Goodvibes

SVGDIR=data/icons/src
ICONDIR=data/icons/hicolor
SITEDIR=docs/goodvibes.readthedocs.io

usage() {
    local status=$1

    (( $status != 0 )) && exec >&2

    echo "Usage: $(basename $0) TARGET..."
    echo
    echo "Build PNG images from their SVG sources."
    echo "Targets can be: all, icons, site"
    echo

    exit $status
}

fail() {
    echo >&2 "$@"
    exit 1
}

step() {
    echo "$(tput bold)⏵ $@$(tput sgr0)"
}

checkcmds() {
    local cmd=
    while (( $# )); do
        cmd=$1; shift
        command -v $cmd >/dev/null 2>&1 && continue
        fail "Command '$cmd' is not installed, aborting"
    done
}

checkdirs() {
    local dir=
    while (( $# )); do
        dir=$1; shift
        [ -d "$dir" ] && continue
        fail "Directory '$dir' does not exist, aborting"
    done
}

do_icons() {
    step 'Building small icons'
    for size in 16 22 24 32 48; do
        inkscape \
            --export-area-page \
            --export-width $size \
            --export-png $ICONDIR/${size}x${size}/apps/$APPID.png \
            $SVGDIR/goodvibes-small.svg
    done

    step 'Building large icons'
    for size in 256 512; do
	inkscape \
            --export-area-page \
            --export-width $size \
	    --export-png $ICONDIR/${size}x${size}/apps/$APPID.png \
            $SVGDIR/goodvibes-large.svg
    done

    step 'Copying symbolic icons'
    cp -v $SVGDIR/goodvibes-symbolic.svg $ICONDIR/symbolic/apps/$APPID-symbolic.svg
}

do_site() {
    local tmpdir=

    tmpdir=$(mktemp --directory --tmpdir=$(pwd) favicon.XXXXXX)
    trap "rm -fr $tmpdir" EXIT

    step 'Building favicon'
    for size in 16 24 32 48 64; do
	inkscape \
            --export-area-page \
            --export-width $size \
            --export-png $tmpdir/$size.png \
	    $SVGDIR/goodvibes-favicon.svg
    done
    convert $tmpdir/*.png $SITEDIR/images/favicon.ico
    identify $SITEDIR/images/favicon.ico

    step 'Building logo'
    inkscape \
	--export-area-page \
	--export-width 192 \
	--export-png $SITEDIR/images/goodvibes.png \
        $SVGDIR/goodvibes-large.svg
}

checkcmds convert identify inkscape
checkdirs "$SVGDIR" "$ICONDIR" "$SITEDIR"

(( $# )) || usage 0

while (( $# )); do
    case "$1" in
        (all)    do_icons; do_site; ;;
        (icons)  do_icons ;;
        (site)   do_site  ;;
        (--help) usage 0  ;;
        (*)      usage 1  ;;
    esac
    shift
done
