counter=0
for ((i = 0; i <= $1; i++))
do
	fname='/sys/devices/system/memory/memory'$i'/removable'
	if [ -f $fname ]
	 then
		x=`cat $fname`
		if [ $x -eq '1' ]
			 then
				counter=$(( counter + 1 ))
				echo "memory"$i
		fi
	fi
done
echo $counter
