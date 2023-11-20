#!/bin/bash

len=$#

if [ $len -ne 1 ]; then
	echo "Numarul de arg nu e ok"
fi

counter=0

while read -r line; do 
	counter=`expr $counter + 1`
done < /dev/stdin

echo $counter


