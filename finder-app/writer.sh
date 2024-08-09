#!/bin/sh

if [ "$#" -ne 2 ]
then
	echo "usage error: $0 <file_path> <string_to_be_written>"
	exit 1
fi

writefile=$1
writestr=$2
createfile="touch $1"
#wfile="echo $writestr > $writefile"

dir_path=$(dirname "$writefile")
file=$(basename "$writefile")

#echo "$dir_path"
#echo "$file"
#exit 0

if [ ! -d "$dir_path" ]
then
	mkdir -p $dir_path
	$createfile
	echo $writestr > $writefile
elif [ ! -f "$file" ]
then
	$createfile
	echo $writestr > $writefile
else
	echo $writestr > $writefile
fi

if [ ! -f "$writefile" ]
then
	echo "file could not be created."
	exit 1
fi
