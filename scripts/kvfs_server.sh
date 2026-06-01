pkill -9 fsserver
/scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 16384M
/scratch/nvm/stamnos/libfs/build/src/kvfs/tool/kvfs create -p /tmp/stamnos_pool -s 16000M -t mfs
/scratch/nvm/stamnos/libfs/build/src/kvfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool &
