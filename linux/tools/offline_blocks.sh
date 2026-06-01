for ((i = 0; i <= $1; i++))
do
	fname='/sys/devices/system/memory/memory'$i'/state'
	if [ -f $fname ]
	 then
		x=`cat $fname`
		echo $fname $x
	fi
done
