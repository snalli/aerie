iter=5
totaliops=0

function cleanup {
	cd /scratch/nvm/stamnos/kernelmode/scmdisk/
	/scratch/nvm/stamnos/kernelmode/scmdisk/fs.sh -r &> /dev/null
	/scratch/nvm/stamnos/kernelmode/scmdisk/fs.sh -c &> /dev/null
	cd /scratch/nvm/file*/
	rm -rf /mnt/scmfs/logfile* &> /dev/null
	rm -rf /mnt/scmfs/bigfile* &> /dev/null
	rm -rf /mnt/scmfs/largefile* &> /dev/null
	echo 3 >> /proc/sys/vm/drop_caches
}

cd /scratch/nvm/stamnos/kernelmode/scmdisk/
/scratch/nvm/stamnos/kernelmode/scmdisk/dev.sh &> /dev/null
/scratch/nvm/stamnos/kernelmode/scmdisk/fs.sh -c &> /dev/null
cd /scratch/nvm/file*/

exec &> scmdisk-results.txt

echo ""
echo "fileserver"
echo "**********"

for (( i = 0 ; i < $iter ; i++ ))
do
	cleanup
	result=`./filebench -f s-config/fileserver.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done


echo ""
echo "randomread"
echo "**********"

for (( i = 0 ; i < $iter ; i++ ))
do
	cleanup
	result=`./filebench -f s-config/randomread.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done

echo ""
echo "webserver"
echo "*********"

for (( i = 0 ; i < $iter ; i++ ))
do
	cleanup
	result=`./filebench -f s-config/webserver.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done

echo ""
echo "webproxy"
echo "********"

for (( i = 0 ; i < $iter ; i++ ))
do
	cleanup
	result=`./filebench -f s-config/webproxy.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done

echo ""
echo "seqread"
echo "*******"

for (( i = 0 ; i < $iter ; i++ ))
do
	cleanup
	result=`./filebench -f s-config/seqread.f | grep "IO Summary"`
	iops=`echo $result | cut -d, -f2 | cut -d\  -f2`
	echo $iops
done
