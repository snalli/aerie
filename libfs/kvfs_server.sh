pkill -9 fsserver
/scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 1024M
/scratch/nvm/stamnos/libfs/build/src/kvfs/tool/kvfs create -p /tmp/stamnos_pool -s 900M -t mfs
/scratch/nvm/stamnos/libfs/build/src/kvfs/server/fsserver -p 10000 -d 5 -s /tmp/stamnos_pool &
