#!/bin/bash
# vim: et sts=4 sw=4

set -e
set -u

MODE=""     # $1
FILES=""
C_FILES=""
H_FILES=""

source $(dirname $0)/lib-git.sh

usage() {
    local status=$1

    (( $status != 0 )) && exec >&2

    echo "Usage: $0 MODE [FILES...]"
    echo
    echo "Modes:"
    echo "  all          Indent the whole source tree"
    echo "  files        Indent files given in argument"
    echo "  staged       Indent git staged files"

    exit $status
}

fail() {
    echo >&2 "$@"
    exit 1
}

checkcmd() {
    command -v $1 >/dev/null 2>&1
}

do_remove_untracked_files()
{
    FILES_ORIG="$FILES"
    FILES=""

    for file in $FILES_ORIG; do
	if git_is_tracked $file; then
	    FILES="$FILES $file"
	fi
    done
}

do_remove_nonexisting_files()
{
    FILES_ORIG="$FILES"
    FILES=""

    for file in $FILES_ORIG; do
	if [ -f $file ]; then
	    FILES="$FILES $file"
	fi
    done
}

do_split_files()
{
    for file in $FILES; do
	if [[ $file = *.c ]]; then
	    C_FILES="$C_FILES $file"
	elif [[ $file = *.h ]]; then
	    H_FILES="$H_FILES $file"
	fi
    done
}

do_indent()
{
    if [ -z "$FILES" ]; then
	echo "No input files"
	return
    fi

    echo "‣ Removing trailing whitespaces..."
    sed -i 's/[ \t]*$//' $FILES

    echo "‣ Indenting..."
    indent \
        --indent-level 8 \
        --use-tabs \
        --braces-on-if-line \
        --braces-after-func-def-line \
        --braces-after-struct-decl-line \
        --cuddle-else \
        --line-length 90 \
        --no-space-after-function-call-names \
        --pointer-align-right \
        --space-after-cast \
        --spaces-around-initializers \
        $FILES
}

do_indent_astyle()    # deprecated
{
    if [ -z "$FILES" ]; then
	echo "No input files"
	return
    fi

    do_split_files

    echo "‣ Removing trailing whitespaces..."
    sed -i 's/[ \t]*$//' $FILES

    # A few words about options...
    #
    # - 'pad-oper' can't be used, since it breaks GQuark definition.
    # For example:
    #     G_DEFINE_QUARK(gszn-error-quark, gszn_error)
    # becomes:
    #     G_DEFINE_QUARK(gszn - error - quark, gszn_error)
    #
    # - 'indent-preproc-define' broke one file lately, so good-bye.

    if [ -n "$C_FILES" ]; then
	echo "‣ Indenting code..."
	astyle --suffix=none           \
	       --formatted             \
	       --style=linux           \
	       --indent=tab=8          \
	       --indent-labels         \
	       --pad-header            \
	       --pad-comma             \
	       --align-pointer=name    \
	       --convert-tabs          \
	       --max-code-length=100   \
	       $C_FILES
    fi

    if [ -n "$H_FILES" ]; then
	echo "‣ Indenting headers..."
	astyle --suffix=none           \
	       --formatted             \
	       --style=linux           \
	       --indent=tab=8          \
	       --pad-comma             \
	       --align-pointer=name    \
	       --convert-tabs          \
	       --max-code-length=100   \
	       --max-instatement-indent=100 \
	       $H_FILES
    fi
}

# Check for proper usage
[ $# -eq 0 ] && usage 0

# Check for commands
checkcmd indent || fail "Please install indent"

# Do the job
MODE=$1
shift
case $MODE in
    (all)
	[ $# -eq 0 ] || usage 1
	FILES="$(find -name '*.[ch]' | tr '\n' ' ')"
	do_remove_untracked_files
	do_indent
	;;

    (files)
	[ $# -eq 0 ] && usage 1
	FILES="$@"
	do_remove_nonexisting_files
	do_indent
	;;

    (staged)
	[ $# -eq 0 ] || usage 1
	FILES="$(git_list_staged)"
	do_indent
	;;

    (*)
	usage 1
	;;
esac
