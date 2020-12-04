#!/bin/bash
# vim: et sts=4 sw=4

# Print the list of translators

set -e
set -u

# translators that are discarded
BLACKLIST=(
    "anonymous"
    "Arnaud Rebillout"
)

# this is the minimum number of additions that a translator
# must have made to be part of the credits.
MIN_LINES_ADDED=20

LCS=()
LANGS=()
NAMES=()
EMAILS=()

fail() {
    echo >&2 "$@"
    exit 1
}

usage() {
    echo "$0 <code/doc>"
    exit 1
}

git_get_lines_added() {

    # get the number of additions by an author on a file
    # https://stackoverflow.com/a/7010890/776208

    local file=$1
    local name=$2

    git log --author="$name" --pretty=tformat: --numstat "$file" \
        | awk '{ add += $1 } END { printf "%s\n", add }'
}

git_get_authors() {

    # get the list of authors for a given file
    # output: name${sep}email

    local file=$1
    local sep=$2

    git log --pretty=format:"%an${sep}%ae" "$file" | sort -u
}

git_get_names() {

    # get the list of authors for a given file
    # output: names

    local file=$1

    git log --pretty=format:"%an" "$file" | sort -u
}

get_translators_git() {

    # get the list of translators from git logs

    local file=$1
    local lc=
    local lang=
    local delete_names=
    local names=
    local name=
    local authors=
    local emails=
    local i=

    # get lc and language from po file
    lc=$(basename "$file" | cut -d'.' -f1)
    lang=$(sed -En 's/"Language-Team: (.*) <.*/\1/p' "$file")

    # create the blacklist, aka names of authors we don't keep
    delete_names=("${BLACKLIST[@]}")
    mapfile -t names < <(git_get_names "$file")
    for name in "${names[@]}"; do
        local lines_added=
        lines_added=$(git_get_lines_added "$file" "$name")
        if [[ $lines_added -lt $MIN_LINES_ADDED ]]; then
            delete_names+=("$name")
        fi
    done
    #declare -p delete_names

    # get the list of authors, and filter it out
    authors=$(git_get_authors "$file" "|")
    for name in "${delete_names[@]}"; do
        authors=$(echo "$authors" | grep -v "$name")
    done

    # move the results to arrays
    mapfile -t names  < <(echo "$authors" | cut -d'|' -f1)
    mapfile -t emails < <(echo "$authors" | cut -d'|' -f2)
    #declare -p names
    #declare -p emails

    # add results to global arrays
    for ((i=0; i<${#names[@]}; i++)); do
        # capitalize 1st letter of each word
        # https://stackoverflow.com/a/1538818/776208
        name=$(echo ${names[$i]} | sed 's/\b\(.\)/\u\1/g')
        LCS+=("$lc")
        LANGS+=("$lang")
        NAMES+=("$name")
        EMAILS+=("${emails[$i]}")
    done
}

get_translators_po_file() {

    # get the list of translators from po files  --  DEPRECATED
    # This method can only output the last person who authored, which
    # is not representative of the work that was done on the po file.

    local file=$1

    lc=$(basename $file | cut -d'.' -f1)

    line=$(grep '^"Language-Team' $file)
    value=$(echo $line | cut -d':' -f2- | sed 's/^ *//')
    lang=$(echo $value | cut -d'<' -f1 | sed 's/ *$//')

    line=$(grep '^"Last-Translator' $file)
    value=$(echo $line | cut -d':' -f2- | sed 's/^ *//')
    name=$(echo $value | cut -d'<' -f1 | sed 's/ *$//')
    email=$(echo $value | cut -d'<' -f2- | cut -d'>' -f1)

    LCS+=("${lc:?}")
    LANGS+=("${lang:?}")
    NAMES+=("${name:?}")
    EMAILS+=("${email:?}")

    #echo "$name <$email> ($lc) ($lang)"
}

get_translators() {

    # get the list of translators (populate global variables)

    local file=

    for file in po/*.po; do
        get_translators_git "$file"
        #get_translators_po_file "$file"
    done
}

output_code() {

    # C-style output, to be pasted directly in the code

    local len=${#NAMES[@]}

    for ((i=0; i<$len; ++i)); do
	local lc=${LCS[$i]}
	local lang=${LANGS[$i]}
	local name=${NAMES[$i]}
	local email=${EMAILS[$i]}
	[ $i -lt $(expr $len - 1) ] \
            && echo -e "\t\"$name <$email> - $lang ($lc)\\\n\" \\" \
            || echo -e "\t\"$name <$email> - $lang ($lc)\";"
    done
}

output_doc() {

    # RST-style output, to be pasted directly in the doc

    local len=${#NAMES[@]}

    for ((i=0; i<$len; ++i)); do
	local lc=${LCS[$i]}
	local lang=${LANGS[$i]}
	local name=${NAMES[$i]}
	local email=${EMAILS[$i]}
	echo " * $name - $lang ($lc)"
    done
}

[ $# -eq 1 ] || usage
[ -d "po"  ] || fail "'po' directory not found."

get_translators

case "$1" in
    code)
	echo "-------- 8< --------"
	output_code
	echo "-------- >8 --------"
	;;
    doc)
	echo "-------- 8< --------"
	output_doc
	echo "-------- >8 --------"
	;;
    *)
	usage
	;;
esac
