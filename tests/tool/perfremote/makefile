PERFPATH="/home/volos/workspace/aerie/kernelmode/linux-3.9/tools/perf/perf"

CCFLAGS=-D_PERF_PATH="\"$(PERFPATH)\""

all: perfremote

perfremote: main.cc monitor.cc
	g++ main.cc monitor.cc ${CCFLAGS} -o perfremote

clean:
	rm perfremote
