#!/bin/bash -e

SPDX_LICENSE_ID=GPL-3.0-only

source $(dirname $0)/lib-git.sh

print_usage()
{
    cat << EOF
Usage: $(basename $0) <command> [<file>...]

Tool to check/add standard headers in C files.

Commands:
  check        Check if source files have a standard header.
  add          If standard header is missing, add it.
EOF
}

unit_name()
{
    echo "Goodvibes Radio Player"
}

do_check()
{
    local file="$1"
    local unit="$(unit_name "$file")"

    head -n 4 "$file" | tr -d '\n' | \
	grep -q -F "/* * $unit * * Copyright (C)"
}

do_add()
{
    local file="$1"
    local unit="$(unit_name "$file")"

    cat << EOF > "$file.tmp"
/*
 * $unit
 *
 * Copyright (C) $(date +%Y) $(git config user.name)
 *
 * SPDX-License-Identifier: ${SPDX_LICENSE_ID:?}
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

$(cat $file)
EOF

    mv "$file.tmp" "$file"
}

# Check for help arguments
[ "$1" == "-h" -o "$1" == "--help" ] && \
    { print_usage; exit 0; }

# Check for proper usage
[ $# -lt 1 ] && \
    { print_usage; exit 1; }

# Ensure we're in the right directory
[ -d "src" ] || \
    { echo >&2 "Please run from root directory"; exit 1; }

# File list
if [ $# -eq 1 ]; then
    filestmp=$(find src -name \*.[ch])
    files=""
    for file in $filestmp; do
        if ! git_is_tracked $file; then
            continue
	fi
	files+="$file\n"
    done
else
    files="${@:2}"
fi

# Do the job
case $1 in
    check)
	for f in $files; do
	    if ! do_check $f; then
		echo "'$f': no standard header found."
	    fi
	done
	;;

    add)
	for f in $files; do
	    if do_check $f; then
		continue;
	    fi

	    echo "'$f': adding standard header."
	    do_add $f
	done
	;;

    *)
	print_usage
	exit 1
	;;
esac
