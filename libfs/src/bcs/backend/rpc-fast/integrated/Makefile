all: rpc/rpclib.o client.out server.out

rpc/rpclib.o: rpc/rpc.cc rpc/rpc.h rpc/marshall.h rpc/rpc_signal.h 
	make -C rpc/

client.out: lock_protocol.* lock_client.* rpc/rpclib.o
	g++ lock_demo.cc lock_client.cc rpc/rpclib.o -Irpc/ -DNUMTHREADS=1 -lrt -o client.out

server.out: lock_server.* lock_smain.* rpc/rpclib.o
	g++ lock_smain.cc lock_server.cc rpc/rpclib.o -Irpc/ -DNUMTHREADS=1 -lrt -o server.out

clean:
	rm -rf *.out [0-9]* ; make -C rpc/ clean

