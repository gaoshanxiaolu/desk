#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Usage: ./make-dsets.sh <input file> <output directory>"
	exit
fi

if [ ! -d $2 ]; then
	mkdir $2
fi


while read -r input || [[ -n $input ]]; do
	s=( $input )
	key=${s[0]}
	hex=${s[1]}
	val=${s[2]}
	len=${#s[@]}
	# Skip commented out lines
	if [ ${key:0:1} != "#" ]; then
		filename=$2/${key}_rw_${hex}.nvm
		if [ $len -gt 3 ]; then
			# To account for spaces in the value field ("A string with spaces")
			echo "Writing: $val... to $filename"
			printf "$val" > $filename
			for i in $(seq 3 $len); do
				# Dont add a space on the final field
				if [ $i -lt $len ]; then
					printf " " >> $filename
					printf "${s[$i]}" >> $filename
				fi
			done
			printf "\x00" >> $filename
		else
			# Normal case with no spaces
			echo "Writing: ${val} to $filename"
			printf "${val}\x00" > $filename
		fi
	fi
done < "$1"
