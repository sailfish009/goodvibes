#!/bin/bash
# vim: et sts=4 sw=4

set -e
set -u

MODE=     # $1
FILES=
C_FILES=
H_FILES=

fail() { echo >&2 "$@"; exit 1; }
checkcmd() { command -v $1 >/dev/null 2>&1; }
assertcmd() { checkcmd "$1" || fail "'$1' is not installed"; }

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


## git helpers

git_is_dirty() {
    test -n "$(git status --porcelain)"
}

git_list_cached_files()
{
    git ls-files '*.[ch]'
}

git_list_staged_files()
{
    git diff --cached --name-only --diff-filter=d | { grep '\.[ch]$' || :; }
}


## do things

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

do_indent_with_gnu_indent()
{
    # DEPRECATED

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

do_indent_with_astyle()
{
    # DEPRECATED

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

do_indent_with_clang_format()
{
    do_split_files

    if [ -z "$C_FILES" ]; then
        echo "No C files"
        return
    fi

    clang-format -i $C_FILES
}

do_indent()
{
    if [ -z "$FILES" ]; then
	echo "No input files"
	return
    fi

    do_indent_with_clang_format
}


## main

assertcmd clang-format

[ $# -eq 0 ] && usage 0
MODE=$1
shift

case $MODE in
    (all)
	[ $# -eq 0 ] || usage 1
	git_is_dirty && fail "Git tree dirty, please cleanup"
        FILES="$(git_list_cached_files)"
	do_indent
	;;

    (files)
	[ $# -eq 0 ] && usage 1
	FILES="$@"
	do_indent
	;;

    (staged)
	[ $# -eq 0 ] || usage 1
	FILES="$(git_list_staged_files)"
	do_indent
	;;

    (*)
	usage 1
	;;
esac
