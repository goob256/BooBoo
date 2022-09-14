# append data to an exe. cpa must be uncompressed

if [ "$1" = "" -o "$2" = "" ] ; then
	echo "Usage: append_data.sh <data.cpa> <game.exe>"
	exit
fi

SIZE=`du -hb $1 | cut -f1 -d'	'`
SIZE=`printf "%08x" $SIZE`
SIZE=`echo $SIZE | sed -e 's/\(..\)\(..\)\(..\)\(..\)/\4\3 \2\1/g'`
echo "0000000: $SIZE" > __tmp__
cat $1 >> $2
xxd -r __tmp__ >> $2
rm __tmp__