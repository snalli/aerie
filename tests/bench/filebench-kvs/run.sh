iter=5
totaliops=0

function cleanup2 {
	cd /scratch/nvm/stamnos/kernelmode/scmdisk/
	/scratch/nvm/stamnos/kernelmode/scmdisk/fs.sh -r &> /dev/null
	/scratch/nvm/stamnos/kernelmode/scmdisk/fs.sh -c &> /dev/null
	rm -rf /mnt/scmfs/logfile* &> /dev/null
	rm -rf /mnt/scmfs/bigfile* &> /dev/null
	rm -rf /mnt/scmfs/largefile* &> /dev/null
	echo 3 >> /proc/sys/vm/drop_caches
	cd /scratch/nvm/stamnos/bench/filebench-scmdisk/
}

function cleanup {
	rm /tmp/kvs.*
}


#exec &> scmdisk-results.txt
exec &

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
