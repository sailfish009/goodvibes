#!/bin/bash -e

# Print the list of translators

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

get_translators() {
    # xargs here is used to trim whitespaces
    # https://stackoverflow.com/a/12973694/776208

    for file in po/*.po; do
	lc=$(basename $file | cut -d'.' -f1)

	line=$(grep '^"Language-Team' $file)
	lang=$(echo $line | cut -d':' -f2- | cut -d'<' -f1 | xargs)

	line=$(grep '^"Last-Translator' $file)
	name=$(echo $line | cut -d':' -f2- | cut -d'<' -f1 | xargs)
	email=$(echo $line | cut -d'<' -f2- | cut -d'>' -f1 | xargs)

	LCS+=("${lc:?}")
	LANGS+=("${lang:?}")
	NAMES+=("${name:?}")
	EMAILS+=("${email:?}")

	#echo "$name <$email> ($lc) ($lang)"
    done
}

output_code() {
    # C style output, to be pasted directly in the code
    local len=${#NAMES[@]}
    for ((i=0; i<$len; ++i)); do
	local lc=${LCS[$i]}
	local lang=${LANGS[$i]}
	local name=${NAMES[$i]}
	local email=${EMAILS[$i]}
	[ $i -lt $(expr $len - 1) ] && \
	    echo -e "\t\"$name <$email> - $lang ($lc)\\\n\" \\" || \
	    echo -e "\t\"$name <$email> - $lang ($lc)\";"
    done
}

output_doc() {
    # rst output
    local len=${#NAMES[@]}
    for ((i=0; i<$len; ++i)); do
	local lc=${LCS[$i]}
	local lang=${LANGS[$i]}
	local name=${NAMES[$i]}
	local email=${EMAILS[$i]}
	echo " * $name - $lang ($lc)"
    done
}

[ -d "po" ] || fail "'po' directory not found."
[ $# -eq 1 ] || usage

get_translators

case $1 in
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
