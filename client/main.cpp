#include "Client.h"

int main(int argc, char **argv) {
    char name[MAXLINE];
    Client c;
    while(1) {
        puts("Please input the server address or name");
        cin >> name;
        if(c.getServerAddr(name))
            break;
        else
            c.showMesg("Can't recognize the server address", NOERROR);
    }
    c.connectToServer();
    c.clientChatting();
}
