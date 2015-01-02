#ifndef SERVER_H
#define SERVER_H

#include "../base.h"

struct cmpSock {
    bool operator() (struct sockaddr_in k1, sockaddr_in k2) {
        return k1.sin_port < k2.sin_port;
    }
};

class Server : public Base {
public:
    Server();
    ~Server();

    /*wait for chatting*/
    void waitChatting();

private:
    /*socket of server address*/
    struct sockaddr_in servAddr;

    /*listen fd*/
    int listenFd;

    /*a buf*/
    char buf[MAXLINE];

    /*client address to opposite client address: ip:port  <-->  ip:port*/
    map<struct sockaddr_in, struct sockaddr_in, cmpSock> Opposite;
    
    /*current sockaddr --> current fd*/
    map<struct sockaddr_in, int, cmpSock> currentFd;
    
    /*name to address: name --> ip:port*/
    map<string, struct sockaddr_in> IPAddr;

    /*child signal funciton, not used*/
    void sig_chld(int signo);

    /*register name*/
    int handleRegister(struct sockaddr_in, int);

    /*forwarding message from one side to another side*/
    void forwardingMesg(struct sockaddr_in, int, struct sockaddr_in, int);

    /*wrap function to writen, write message to socket*/
    void WriteFd(string, int);
};
#endif //SERVER_H
