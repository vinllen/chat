#ifndef CLIENT_H
#define CLIENT_H

#include "../base.h"

class Client : public Base {
public:
    Client();
    ~Client();

    /*start chatting*/
    void clientChatting();
   
    /*change host name to ip*/
    bool getServerAddr(char *);

    /*try to build connection*/
    bool connectToServer();

private:
    /*server address*/
    struct sockaddr_in servAddr;
    
    /*fd*/
    int sockFd;

    /*embedded pipe fd*/
    int pipefd[2];

    /*not useful, local arrary*/
    char tmpMesg[MAXLINE];

    /*server ip*/
    char servIP[MAXLINE];

    /*change host name to ip*/
    bool hostname_to_ip(char *, char *);

    /*read from buf, actually, is from stdin*/
    int readFromFP(char *, int );

    /*judge if current message is send file, if yes, receive it*/
    int recvIfFile(int, char *);

    /*receive file*/
    ssize_t do_recvfile(int out_fd, int in_fd, off_t offset, size_t count);
};

#endif //CLIENT_H
