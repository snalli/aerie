all: rpclib.o

rpclib.o: *.h *.cc
	g++ -c rpc.cc -o rpclib.o -lpthread -lrt -DNOPAPI -DNUMTHREADS=1 -DRPCS_VER_2 -DRPC_LIB

clean: 
	rm rpclib.o