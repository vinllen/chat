#include "Server.h"

Server::Server() {
    /*clear map*/
    Opposite.clear();
    currentFd.clear();
    IPAddr.clear();

    /*tcp server start */
    listenFd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(SERV_PORT);

	bind(listenFd, (struct sockaddr *) &servAddr, sizeof(servAddr));

	listen(listenFd, LISTENQ);
}

Server::~Server() {
    close(listenFd);
}

void Server::waitChatting() {
    int	connFd; //connected fd
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in clientAddr;
    for(;;) {
        clilen = sizeof(clientAddr);
        if ( (connFd = accept(listenFd, (struct sockaddr *) &clientAddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				showMesg("Error happen when accept!", HAVEERROR);
		}
        while(1) {
            int x = handleRegister(clientAddr, connFd);
            if(x == 1) { // 1 means connected
                /*fork a process*/
                if ( (childpid = fork()) == 0) {
    		    	close(listenFd);
                    forwardingMesg(clientAddr, connFd, Opposite[clientAddr], currentFd[Opposite[clientAddr]]);
				    showMesg("Connection end.", HAVEERROR);
	        	}
                else { //father process
                    close(connFd);
                    break;
                }
            }
            else if(x == 0) // 0 means one side has been registered
                break;
        }
    }
}

/*
 *0: register one side
 *-1: illegal
 *1: success 
*/
int Server::handleRegister(struct sockaddr_in clientAddr, int sockFd) {
    int n;
    char first[MAXLINE], second[MAXLINE], third[MAXLINE];

    /*read from fd but is none, means illegal*/
    if ( (n = readline(sockFd, buf, MAXLINE)) == 0) {
        if(Opposite.find(clientAddr) != Opposite.end()) {
             shutdown(currentFd[Opposite[clientAddr]], SHUT_WR);	/* send FIN */
        }
		return -1;
    }
    buf[n] = '\0';
    sscanf(buf, "%s", first);
    /*register message*/
    if(strcmp(first, "[register]") == 0) {
        sscanf(buf, "%s %s %s", first, second, third);
        /*address is not register */
        if(IPAddr.find(second) == IPAddr.end()) {
            IPAddr[second] = clientAddr;
            currentFd[clientAddr] = sockFd;
            WriteFd("Regist successfully\n", sockFd);
            /*opposite address is not register*/
            if(IPAddr.find(third) == IPAddr.end()) {
                WriteFd("Wait for opposite\n", sockFd);
                return 0;
            }
            else { /*opposite has been registered*/
                /*opposite's registered name is not equal to input*/
                if(Opposite.find(clientAddr) != Opposite.end() || \
                    Opposite.find(IPAddr[third]) != Opposite.end()) {
                    WriteFd("Illegal opposite, one name only for one opposite.\n", sockFd);
                    return -1;
                }
                
                /*build connection*/
                Opposite[clientAddr] = IPAddr[third];
                Opposite[IPAddr[third]] = clientAddr;
                WriteFd("Connected!\n", sockFd);
                WriteFd("Connected!\n", currentFd[IPAddr[third]]);
                return 1;
            }
        }
        else { /*address has been registered, illegal*/
            WriteFd("Already registered!\n", sockFd);
            return -1;
        }
    }
    else { /*message is not register*/
        /*opposite is not register*/
        if(Opposite.find(clientAddr) == Opposite.end()) {
            WriteFd("Need register first\n", sockFd);
            return -1;
        }
    }
    return 0;

}

void Server::forwardingMesg(struct sockaddr_in clientAddr_1, int sockFd_1, struct sockaddr_in clientAddr_2, int sockFd_2) {
    int	maxfd, stdineof;
	fd_set rset;
	int n;
    
	FD_ZERO(&rset);
    /*use "select" method to forwarding message*/ 
    for ( ; ; ) {
		FD_SET(sockFd_1, &rset);
		FD_SET(sockFd_2, &rset);
		maxfd = max(sockFd_1, sockFd_2) + 1;
		select(maxfd, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockFd_1, &rset)) {	/* socket is readable */
			if ( (n = read(sockFd_1, buf, MAXLINE)) == 0) {
                shutdown(sockFd_2, SHUT_WR);
                showMesg("Connection terminate.", HAVEERROR);
			}

			writen(sockFd_2, buf, n);
		}

		if (FD_ISSET(sockFd_2, &rset)) {	/* socket is readable */
			if ( (n = read(sockFd_2, buf, MAXLINE)) == 0) {
                shutdown(sockFd_1, SHUT_WR);	/* send FIN */
                showMesg("Connection terminate.", HAVEERROR);
			}

			writen(sockFd_1, buf, n);
		}
	}

}

void Server::WriteFd(string mesg, int sockFd) {
    writen(sockFd, mesg.c_str(), mesg.size());
}

void Server::sig_chld(int signo)
{
	pid_t pid;
	int stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}
