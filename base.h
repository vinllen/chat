#ifndef BASE_H
#define BASE_H

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h> //hostent

#include "errno.h"
#include <string>
#include <map>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

#define HAVEERROR 1 
#define NOERROR 0
const int SERV_PORT = 11223;
const int MAXLINE = 1 << 12;
const int LISTENQ = 1 << 10;

class Base {
public:
    /*show message include errors*/
	void showMesg(string x, bool isError);

protected:
    /*wrap function to write n letters to socket, server call*/
	ssize_t writen(int fd, const void *vptr, size_t n);

    /*wrap function to read n letters from socket*/
	ssize_t readline(int fd, void *vptr, size_t maxlen);

    /*change int to string*/
	string toString(int x);

private:
    ssize_t my_read(int fd, char *ptr);
	int	read_cnt;
	char *read_ptr;
	char read_buf[MAXLINE];
};

#endif // BASE_H
