#!/bin/bash 
KCPATH=/home/volos/workspace/aerie/bench/kyotocabinet-1.2.76
export LIBS="-Wl,-rpath=$KCPATH $KCPATH/libkyotocabinet.so"
export CPPFLAGS="-I$KCPATH -DKVS"
./configure
