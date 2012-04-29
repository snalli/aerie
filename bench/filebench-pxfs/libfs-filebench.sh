iter=5
totaliops=0

function restart_server {
	tmp=`/scratch/nvm/stamnos/libfs/pxfs_server.sh &> /dev/null`
	echo 3 >> /proc/sys/vm/drop_caches
}

exec &> filebench-pxfs.rst

echo "fileserver"
echo "**********"

for (( i = 0 ; i < $iter ; i++ ))
do
	restart_server
	result=`./../bench/file*/filebench -f ../bench/file*/s-config/fileserver.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done


echo "randomread"
echo "**********"

for (( i = 0 ; i < $iter ; i++ ))
do
	restart_server
	result=`./../bench/file*/filebench -f ../bench/file*/s-config/randomread.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done

echo "webserver"
echo "*********"

for (( i = 0 ; i < $iter ; i++ ))
do
	restart_server
	result=`./../bench/file*/filebench -f ../bench/file*/s-config/webserver.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done

echo "webproxy"
echo "********"

for (( i = 0 ; i < $iter ; i++ ))
do
	restart_server
	result=`./../bench/file*/filebench -f ../bench/file*/s-config/webproxy.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done

echo "seqread"
echo "*******"

for (( i = 0 ; i < $iter ; i++ ))
do
	restart_server
	result=`./../bench/file*/filebench -f ../bench/file*/s-config/seqread.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done
