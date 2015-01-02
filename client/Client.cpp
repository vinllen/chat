#include "Client.h"

Client::Client() {
}

Client::~Client() {
    close(sockFd);
}

/*host will be given by input*/
bool Client::getServerAddr(char *hostname) {
     return hostname_to_ip(hostname, servIP);
}

/*try to connect to server, return 0/1*/
bool Client::connectToServer() {
    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, servIP, &servAddr.sin_addr);

    if(connect(sockFd, (sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
       showMesg("Connect failed", HAVEERROR); 
       return 0;
    }
    showMesg("Connect to server successfully", NOERROR);
    return 1;
}

/*start chatting*/
void Client::clientChatting() {
    int	maxfd, stdineof;
	fd_set rset;
	char buf[MAXLINE];
	int n;
    FILE *fp = stdin;

	stdineof = 0;
	FD_ZERO(&rset);
	for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockFd, &rset);
		maxfd = max(fileno(fp), sockFd) + 1;
		select(maxfd, &rset, NULL, NULL, NULL);

        /*receive message from socket*/
		if (FD_ISSET(sockFd, &rset)) {	/* socket is readable */
			if ( (n = read(sockFd, buf, MAXLINE)) == 0) {
				if (stdineof == 1)
					return;		/* normal termination */
				else
					showMesg("Server terminated", HAVEERROR);
			}
            buf[n] = '\0';
            /*judge wether this message is send file message*/
            int ifFile = recvIfFile(sockFd, buf);
            if(ifFile == -1)
                showMesg("The file with the same name was exist or can't write.", HAVEERROR);
            else if(!ifFile)
    			writen(fileno(stdout), buf, n);
		}
        
        /*receive message from stdin*/
		if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
			if ( (n = read(fileno(fp), buf, MAXLINE)) == 0) {
				stdineof = 1;
				shutdown(sockFd, SHUT_WR);	/* send FIN */
				FD_CLR(fileno(fp), &rset);
				continue;
			}
            
            int x = readFromFP(buf, n);
			if(x == 0) { //quit
                stdineof = 1;
				shutdown(sockFd, SHUT_WR);	/* send FIN */
				FD_CLR(fileno(fp), &rset);
				continue;
            }
            //writen(sockFd, buf, n);
		}
	}
    fclose(fp);
    showMesg("chatting end.", NOERROR);
}

/*read message from stdin*/
int Client::readFromFP(char *buf, int n) {
    buf[n] = '\0';
    char first[MAXLINE], second[MAXLINE];
    string fileMesg = "[file]";

    sscanf(buf, "%s", first);
    /*message is a file*/
    if(strcmp(first, "[file]") == 0) {
        struct stat stat_buf;      /* argument to fstat */

        sscanf(buf, "%s %s", first, second);
        int fileFp = open(second, O_RDONLY);
        if(fileFp == -1) {
            sprintf(tmpMesg, "unable to open '%s': %s", second, strerror(errno));
            showMesg(tmpMesg, HAVEERROR);
        }
        
        fstat(fileFp, &stat_buf);
       
        /*first, we should send a message to tell the opposite to prepare to receive file, the message formate: [file] [name] [bytes]*/
        fileMesg += " ";
        fileMesg += second;
        fileMesg += " ";
        fileMesg += toString(stat_buf.st_size);
        writen(sockFd, fileMesg.c_str(), fileMesg.size());
        
        /*use system call: "sendfile" */
        off_t offset = 0;  /* copy file using sendfile */
        int nr = sendfile(sockFd, fileFp, &offset, stat_buf.st_size);
        if(nr != stat_buf.st_size)
            showMesg("incomplete transfer from sendfile", HAVEERROR);
        cout << "size:" << nr << endl;
        sprintf(tmpMesg, "Send file: %s successfully", second);
        showMesg(tmpMesg, NOERROR);
        
        close(fileFp);
    }
    else if(strcmp(first, "[vedio]") == 0) { //not compelete
    }
    else if(strcmp(first, "[quit]") == 0) { //quit
        return 0;
    }
    else { //general message 
        writen(sockFd, buf, n);
    }
    return 1;
}

/*0: not a file 
 *1: is a file and receive successfully
 *-1: is a file but receive failed
 * */
int Client::recvIfFile(int sockFd, char *buf) {
    char first[MAXLINE], second[MAXLINE];
    int cnt;
    sscanf(buf, "%s", first);
    if(strcmp(first, "[file]") != 0)
        return 0;

    sscanf(buf, "%s %s %d", first, second, &cnt);

    /*create a file with privalige 600*/
    int fileFp = open(second, O_WRONLY | O_CREAT | O_TRUNC, 00600);
    if(fileFp == -1) {
        sprintf(tmpMesg, "unable to open '%s': %s", second, strerror(errno));
        showMesg(tmpMesg, HAVEERROR);
        return -1;
    }
    showMesg("file sending, please waitting");

    off_t offset = 0;
    if ( pipe(pipefd) < 0 ) {
        sprintf(tmpMesg, "pipe error: %s", strerror(errno));
        showMesg(tmpMesg, HAVEERROR);
    }
    int tmp = do_recvfile(fileFp, sockFd, offset, cnt);
    cout << "receive size:" << tmp << endl;
    if(tmp <= 0 || tmp != cnt) {
        sprintf(tmpMesg, "receive file error: %s", strerror(errno));
        showMesg(tmpMesg, HAVEERROR);

        return -1;
    }
    sprintf(tmpMesg, "Receive file: %s successfully", second);
    showMesg(tmpMesg, NOERROR);
    close(fileFp);
    return 1;
}

/*
 * Copyright: Pat Patterson
 * it can receive file from socket, and copy to a file use system call "splice"
 * */
ssize_t Client::do_recvfile(int out_fd, int in_fd, off_t offset, size_t count) {
    ssize_t bytes, bytes_sent, bytes_in_pipe;
    size_t total_bytes_sent = 0;

    // Splice the data from in_fd into the pipe
    while (total_bytes_sent < count) {
        if ((bytes_sent = splice(in_fd, NULL, pipefd[1], NULL,
                count - total_bytes_sent, 
                SPLICE_F_MORE | SPLICE_F_MOVE)) <= 0) {
            if (errno == EINTR || errno == EAGAIN) {
                // Interrupted system call/try again
                // Just skip to the top of the loop and try again
                continue;
            }
            perror("splice");
            return -1;
        }

        // Splice the data from the pipe into out_fd
        bytes_in_pipe = bytes_sent;
        while (bytes_in_pipe > 0) {
            if ((bytes = splice(pipefd[0], NULL, out_fd, &offset, bytes_in_pipe,
                    SPLICE_F_MORE | SPLICE_F_MOVE)) <= 0) {
                if (errno == EINTR || errno == EAGAIN) {
                    // Interrupted system call/try again
                    // Just skip to the top of the loop and try again
                    continue;
                }
                perror("splice");
                return -1;
            }
            bytes_in_pipe -= bytes;
        }
        total_bytes_sent += bytes_sent;
    }
    return total_bytes_sent;
}

/*hostname to ip*/
bool Client::hostname_to_ip(char *hostname , char *ip)
{
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;
 
    if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0) 
    {
        //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 0;
    }
 
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        h = (struct sockaddr_in *) p->ai_addr;
        strcpy(ip , inet_ntoa( h->sin_addr ) );
    }
     
    freeaddrinfo(servinfo); // all done with this structure
    return 1;
}
