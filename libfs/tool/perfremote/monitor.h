#ifndef __MONITOR_H_KAL189
#define __MONITOR_H_KAL189

#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>

const uint64_t MONITOR_PORT = 70000;

class Monitor {
public:
    class Client;
    class Server;
    struct Message;
    struct Payload;
};

struct Monitor::Payload {
    struct Result;
    struct Attach;
    struct Deattach;
};

struct Monitor::Payload::Result {
    static Monitor::Message createMessage(int val);

    int _val;
};

struct Monitor::Payload::Attach {
    static Monitor::Message createMessage(pid_t pid);

    pid_t    _pid;
};

struct Monitor::Payload::Deattach {
    static Monitor::Message createMessage();
};


struct Monitor::Message {
    enum Types {
        RESULT = 1,
        ATTACH,
        DEATTACH
    };
    int _cmd;
    union {
        struct Monitor::Payload::Result    _result;
        struct Monitor::Payload::Attach    _attach;
        struct Monitor::Payload::Deattach  _deattach;
    } _payload;
};


class Monitor::Client {
public:
    Client(int serv_port = MONITOR_PORT);
    void attach();
    int deattach();
    
private:
    int sendCmd(Monitor::Message& Msg);

    int _serv_port;
    int _sock; 
};


class Monitor::Server {
public:
    Server(char* perf_path, int serv_port);
    static Monitor::Server* create(char* perf_path, int serv_port = MONITOR_PORT);
    void run();
    void handleClient(int clnt_sock);
    int execCmdAttach(Monitor::Message& msg);
    int execCmdDeattach(Monitor::Message& msg);

private:
    bool            _run_monitor;
    pthread_t       _monitor_thread;
    pthread_mutex_t _mutex;
    pthread_cond_t  _cond;
    char*           _perf_path;
    int             _serv_port;
    pid_t           _target_pid;
    pid_t           _perf_pid;
};

#endif /* __MONITOR_H_KAL189 */
