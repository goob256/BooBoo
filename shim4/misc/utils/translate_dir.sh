#!/bin/sh

BEGIN_COUNT=$1
CALL=$2
REV_CALL1=$3
REV_CALL2=$4
shift
shift
shift
shift

for f in $* ; do
	if [ -e symbols.txt ] ; then
		COUNT=`wc -l symbols.txt | cut -f1 -d' '`
	else
		COUNT=0
	fi
	COUNT=$((COUNT+BEGIN_COUNT))
	translate $COUNT "$CALL" "$REV_CALL1" "$REV_CALL2" $f
done
