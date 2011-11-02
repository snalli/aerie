#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <assert.h>
#include <string.h>
#include "event.h"

#include <stdio.h>

Event::Event(const char* tag, const char* msg_pathname, const char* sem_pathname)
	: tag_(tag)
{
	assert(AttachToSysVIPC(msg_pathname, sem_pathname) == 0);
}

Event::Event(const char* tag, key_t msg_key, key_t sem_key)
	: tag_(tag)
{
	assert(AttachToSysVIPC(msg_key, sem_key) == 0);
}


int Event::AttachToSysVIPC(const char* msg_pathname, const char* sem_pathname)
{
	key_t msg_key;
	key_t sem_key;
	
	if ((msg_key = ftok(msg_pathname, 1)) < 0) {
		return -1;
	}
	if ((sem_key = ftok(sem_pathname, 1)) < 0) {
		return -1;
	}

	return AttachToSysVIPC(msg_key, sem_key);
}


int Event::AttachToSysVIPC(key_t msg_key, key_t sem_key)
{
	if ((msgid_ = msgget(msg_key, 0)) < 0) {
		return -1;
	}
	if ((semid_ = semget(sem_key, 1, 0)) < 0) {
		return -1;
	}

	printf("semid=%d\n", semid_);
	return 0;
}



int Event::Trigger(const char* event)
{
	char buf[512];
	const char* delim = ".";
	struct sembuf sb = {0, -1, 0};
	struct msgbuf* mb = (struct msgbuf*) &buf;

	mb->mtype = 1;
	sprintf(mb->mtext, "%s%s%s", tag_, delim, event);
	msgsnd(msgid_, mb, strlen(mb->mtext), 0);
	semop(semid_, &sb, 1);
	return 0;
}


main(int argc, char* argv[])
{
	//Event e("/tmp/msg", "/tmp/sem");
	Event e("C1.T1", 42, 43);

	e.Trigger("test");
}
