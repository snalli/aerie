#ifndef __EVENT_H_AKL193
#define __EVENT_H_AKL193

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>

class Event {
public:
	Event(const char* tag, const char* msg_pathname, const char* sem_pathname);
	Event(const char* tag, key_t msg_key, key_t sem_key);
	int Trigger(const char* name);

private:
	int AttachToSysVIPC(const char* msg_pathname, const char* sem_pathname);
	int AttachToSysVIPC(key_t msg_key, key_t sem_key);
	
	int msgid_;
	int semid_;
	const char* tag_;
};


#endif  // __EVENT_H_AKL193
