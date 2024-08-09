#!/bin/sh

if [ "$#" -ne 2 ]
then
	echo "error. usage $0 <drectory_path> <search_string>"
	exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d "$filesdir" ]
then
	echo "directory path non-exixtant."
	exit 1
fi

#count the number of files in the directory and subdirectories
files=$(find "$filesdir" -type f -name '*' | wc -l)

str_match=$(grep -rch "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $files and the number of matching lines are $str_match"
