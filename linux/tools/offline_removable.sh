for ((i = 0; i <= $1; i++))
do
	fname='/sys/devices/system/memory/memory'$i'/removable'
	fname_state='/sys/devices/system/memory/memory'$i'/state'
	if [ -f $fname ]
	 then
		x=`cat $fname`
		x1=`cat $fname_state`
		if [ $x -eq '1' && $x1 == "online" ]
			 then
				echo "memory"$i
		fi
	fi
done
