#include "monitor.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <fcntl.h>
#include "error.h"

Monitor::Message Monitor::Payload::Result::createMessage(int val)
{
    Monitor::Message msg;
    msg._cmd = Monitor::Message::RESULT;
    msg._payload._result._val = val;
    return msg;
}

Monitor::Message Monitor::Payload::Attach::createMessage(pid_t pid)
{
    Monitor::Message msg;
    msg._cmd = Monitor::Message::ATTACH;
    msg._payload._attach._pid = pid;
    return msg;
}

Monitor::Message Monitor::Payload::Deattach::createMessage()
{
    Monitor::Message msg;
    msg._cmd = Monitor::Message::DEATTACH;
    return msg;
}

Monitor::Client::Client(int serv_port)
    : _serv_port(serv_port)
{ }


void Monitor::Client::attach()
{
    struct sockaddr_in serv_addr;

    if ((_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        handle_error("socket() failed");
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(_serv_port);

    if (connect(_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        handle_error("connect() failed");
    } 
    
    std::cerr << "Attach myself (" << getpid() << ") to monitor" << std::endl;  
    Monitor::Message msg = Monitor::Payload::Attach::createMessage(getpid());
    if (sendCmd(msg) != 0) {
        handle_error("setup failed");
    }
}

int Monitor::Client::sendCmd(Monitor::Message& msg)
{
    Monitor::Message reply;

    if (send(_sock, &msg, sizeof(msg), 0) != sizeof(msg)) {
        handle_error("send() failed");
    }
    if (recv(_sock, &reply, sizeof(reply), 0) != sizeof(reply)) {
        handle_error("recv() failed");
    }
    return reply._payload._result._val;
}

int Monitor::Client::deattach()
{
    Monitor::Message msg = Monitor::Payload::Deattach::createMessage();
    sendCmd(msg);
    close(_sock);
    return 0;
}


Monitor::Server::Server(char* perf_path, int serv_port)
    : _run_monitor(false),
      _perf_path(perf_path),
      _serv_port(serv_port)
{ }

Monitor::Server* Monitor::Server::create(char* perf_path, int serv_port)
{
    Monitor::Server* monitor;

    monitor = new Monitor::Server(perf_path, serv_port);
    std::cerr << "Performance Monitor ready!" << std::endl;
    return monitor;
}

void Monitor::Server::run()
{
    int                serv_sock; 
    int                clnt_sock; 
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    unsigned int       clnt_len;
    
    if ((serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        handle_error("socket() failed");
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(_serv_port);
    
    if (bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        handle_error("bind() failed");
    }

    if (listen(serv_sock, 5) < 0) {
        handle_error("listen() failed");
    }
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    for(;;) 
    {
        std::cerr << "Waiting for process to monitor..." << std::endl;
        if ((clnt_sock = accept(serv_sock, 
                                (struct sockaddr *) &clnt_addr, &clnt_len)) < 0) 
        {
            handle_error("accept() failed");
        }
        handleClient(clnt_sock);    
    }
    
}

void Monitor::Server::handleClient(int clnt_sock) 
{
    Monitor::Message msg;
    int  recv_msg_size;
    int  ret;

    for (;;) {
        if ((recv_msg_size = recv(clnt_sock, &msg, sizeof(msg), 0)) < 0) {
            handle_error("recv() failed");
        }
        if (recv_msg_size == 0) { 
            return;
        }
        std::cerr << "COMMAND: " << msg._cmd << std::endl << std::flush;
        switch(msg._cmd) {
            case Monitor::Message::ATTACH:
                ret = execCmdAttach(msg);
                break;
            case Monitor::Message::DEATTACH:
                ret = execCmdDeattach(msg);
                break;
        }
        /* send reply back to client */
        Monitor::Message reply = Monitor::Payload::Result::createMessage(ret);
        if (send(clnt_sock, &reply, sizeof(reply), 0) != sizeof(reply)) {
            handle_error("send() failed");
        }
    }
}

int Monitor::Server::execCmdAttach(Monitor::Message& msg)
{
    _target_pid = msg._payload._attach._pid;

    std::cerr << "Monitor process " << _target_pid << "..." << std::endl << std::flush;

    _perf_pid = fork();
    if (!_perf_pid) { /* child */
	char buf[16];
	sprintf(buf, "-p %d", _target_pid);
        if (execl(_perf_path, "perf", "record", buf, (char*) 0) < 0) {
            std::cerr << "ERROR: Failed to execute perf" << std::endl;
	}
    }

    return 0;
}

int Monitor::Server::execCmdDeattach(Monitor::Message& msg)
{
    kill(_perf_pid, 2);

    return 0;
}
