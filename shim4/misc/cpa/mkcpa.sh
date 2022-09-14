#!/bin/bash

FILES=`find * -type f | sort`

/bin/echo "Writing header..."

# write magic
/bin/echo -n CPA2 > $1

TMPDIRNAME=__tmp__hopefully_you_dont_have_anything_named_this__
C=`pwd`

mkdir ../$TMPDIRNAME
cp -a * ../$TMPDIRNAME
cd ../$TMPDIRNAME

for f in `find . -type f` ; do
	zpipe < $f > $f.deflated
	mv $f.deflated $f
done

ls -l $FILES | awk '{sum += $5} END {print (sum+1)}' >> $1 # gzipped size

/bin/echo "Writing data..."
cat $FILES >> $1 # write gzipped data

/bin/echo "Writing info..."
/bin/echo >> $1 # make filenames neat (sum+1 above adds 1 for this newline)
for f in $FILES ; do
	/bin/echo -n "`ls -l $C/$f | awk '{size = $5;} {printf "%d\n", size}'` " >> $1
	ls -l $f | awk '{size = $5; name = $9} {printf "%d %s\n", size, name}' >> $1
done

cd $C

rm -rf ../$TMPDIRNAME
